//
//  GOMTalking.cpp
//  MrProximity
//
//  Created by Fabian Schmidt on 11/20/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include "curl/curl.h"
#include <boost/asio.hpp>
#include "kinskiCore/Logger.h"
#include "Downloader.h"

using namespace std;

namespace kinski{ namespace net{
    
typedef std::map<CURL*, Downloader::ActionPtr> HandleMap;

struct Downloader_impl
{
    boost::asio::io_service *m_io_service;
    CURLM *m_curl_multi_handle;
    HandleMap m_handle_map;
    
    Downloader_impl(boost::asio::io_service *io):
    m_io_service(io),
    m_curl_multi_handle(curl_multi_init()){};
    virtual ~Downloader_impl(){curl_multi_cleanup(m_curl_multi_handle);}
};
    
class Action
{
protected:
    CURL *m_curl_handle;
    ConnectionInfo m_connection_info;
    Downloader::CompletionHandler m_completion_handler;
    Downloader::ProgressHandler m_progress_handler;
    
    long m_timeout;
    std::vector<uint8_t> m_response;
 
    /*!
     * Callback to process incoming data
     */
    static size_t write_static(void *buffer, size_t size, size_t nmemb,
                               void *userp)
    {
        size_t realsize = size * nmemb;
        if (userp)
        {
            Action *ourAction = static_cast<Action*>(userp);
            uint8_t* buf_start = (uint8_t*)(buffer);
            uint8_t* buf_end = buf_start + realsize;
            ourAction->m_response.insert(ourAction->m_response.end(), buf_start, buf_end);
        }
        return realsize;
    }
    
    /*!
     * Callback for data provided to Curl for sending
     */
    static size_t read_static(void *ptr, size_t size, size_t nmemb,
                              void *inStream)
    {
        size_t length = strlen((char*) inStream);
        memcpy(ptr, inStream, length);
        return length;
    }
    
    /*!
     * Callback for transfer progress
     */
    static int progress_static(void *userp, double dltotal, double dlnow, double ult,
                               double uln)
    {
        Action *self = static_cast<Action*>(userp);
        self->m_connection_info.dl_total = dltotal;
        self->m_connection_info.dl_now = dlnow;
        self->m_connection_info.ul_total = ult;
        self->m_connection_info.ul_now = uln;
        LOG_TRACE << self->connection_info().url << " : " << self->connection_info().dl_now << " / "
        << self->connection_info().dl_total;
        
        if(self->m_progress_handler)
            self->m_progress_handler(self->m_connection_info);
        
        return 0;
    }
    
public:
    Action():
    m_curl_handle(curl_easy_init()),
    m_connection_info({"", 0, 0, 0, 0})
    {
        curl_easy_setopt(m_curl_handle, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(m_curl_handle, CURLOPT_WRITEFUNCTION, write_static);
        curl_easy_setopt(m_curl_handle, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(m_curl_handle, CURLOPT_PROGRESSDATA, this);
        curl_easy_setopt(m_curl_handle, CURLOPT_PROGRESSFUNCTION, progress_static);
    };
    virtual ~Action()
    {curl_easy_cleanup(m_curl_handle);};
    
    const std::vector<uint8_t>& getResponse(){return m_response;};
    
    bool perform()
    {
        CURLcode curlResult = curl_easy_perform(handle());
        if(!curlResult && m_completion_handler) m_completion_handler(m_connection_info, m_response);
		return !curlResult;
    }
    
    CURL* handle() const {return m_curl_handle;}
    ConnectionInfo connection_info() const {return m_connection_info;}
    
    void set_handle(CURL *handle)
    {
        if(m_curl_handle){ curl_easy_cleanup(m_curl_handle); }
        m_curl_handle = handle;
    }
    
    Downloader::CompletionHandler completion_handler() const {return m_completion_handler;}
    void set_completion_handler(Downloader::CompletionHandler ch){m_completion_handler = ch;}
    void set_progress_handler(Downloader::ProgressHandler ph){m_progress_handler = ph;}
};

    
// Timeout interval for http requests
const long Downloader::DEFAULT_TIMEOUT = 1L;

class GetURLAction: public Action
{
private:
	string m_url;
public:
	GetURLAction(const string &the_url) :
    Action(),
    m_url(the_url)
	{
        m_connection_info.url = the_url;
        
        /* specify target URL, and note that this URL should include a file
		 name, not only a directory */
		curl_easy_setopt(handle(), CURLOPT_URL, m_url.c_str());
	}
};

Downloader::Downloader() :
m_impl(new Downloader_impl(NULL)),
m_timeout(DEFAULT_TIMEOUT),
m_running(0)
{
    
}
    
Downloader::Downloader(boost::asio::io_service &io) :
    m_impl(new Downloader_impl(&io)),
    m_timeout(DEFAULT_TIMEOUT),
    m_running(0)
{

}
    
Downloader::~Downloader()
{

}

std::vector<uint8_t> Downloader::getURL(const std::string &the_url)
{
	ActionPtr url_action = make_shared<GetURLAction>(the_url);
    curl_easy_setopt(url_action->handle(), CURLOPT_TIMEOUT, m_timeout);
    LOG_DEBUG << "trying to fetch url: '" << the_url << "'";
	url_action->perform();
	return url_action->getResponse();
}

void Downloader::poll()
{
    if(m_running)
    {
        curl_multi_perform(m_impl->m_curl_multi_handle, &m_running);
        
        CURLMsg *msg;
        int msgs_left;
        CURL *easy;
        CURLcode res;
        
        while ((msg = curl_multi_info_read(m_impl->m_curl_multi_handle, &msgs_left)))
        {
            if (msg->msg == CURLMSG_DONE)
            {
                easy = msg->easy_handle;
                res = msg->data.result;
                
                curl_multi_remove_handle(m_impl->m_curl_multi_handle, easy);
                
                auto itr = m_impl->m_handle_map.find(easy);
                if(itr != m_impl->m_handle_map.end())
                {
                    if(!res)
                    {
                        LOG_DEBUG << "'" << itr->second->connection_info().url << "' completed successfully";
                        if(itr->second->completion_handler())
                        {
                            itr->second->completion_handler()(itr->second->connection_info(),
                                                              itr->second->getResponse());
                        }
                    }
                    else
                    {
                        LOG_DEBUG << "could not retrieve '" << itr->second->connection_info().url << "'";
                    }
                    m_impl->m_handle_map.erase(itr);
                }
            }
        }
        if(m_running && m_impl->m_io_service)
            m_impl->m_io_service->post(std::bind(&Downloader::poll, this));
    }
}
    
void Downloader::async_getURL(const std::string &the_url,
                              CompletionHandler ch,
                              ProgressHandler ph)
{
    LOG_DEBUG << "trying to fetch url: '" << the_url << "' async";
    
    // create an action which holds an easy handle
    ActionPtr url_action = make_shared<GetURLAction>(the_url);
    
    // set options for this handle
    curl_easy_setopt(url_action->handle(), CURLOPT_TIMEOUT, m_timeout);
    
    url_action->set_completion_handler(ch);
    url_action->set_progress_handler(ph);
    m_impl->m_handle_map[url_action->handle()] = url_action;
    
    // add handle to multi
    curl_multi_add_handle(m_impl->m_curl_multi_handle, url_action->handle());
    
    curl_multi_perform(m_impl->m_curl_multi_handle, &m_running);
    
    if(m_impl->m_io_service)
    {
        m_impl->m_io_service->post(std::bind(&Downloader::poll, this));
    }
}

long Downloader::getTimeOut()
{
    return m_timeout;
}
    
void Downloader::setTimeOut(long t)
{
    m_timeout=t;
}
    
}}// namespace