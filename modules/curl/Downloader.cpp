//
//  GOMTalking.cpp
//  MrProximity
//
//  Created by Fabian Schmidt on 11/20/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include "Downloader.h"
#include <string.h>
#include "curl/curl.h"
#include "kinskiCore/Logger.h"

using namespace std;

namespace kinski{ namespace net{

struct Downloader_impl
{
    CURLM *m_curl_multi_handle;
    
    Downloader_impl(): m_curl_multi_handle(curl_multi_init()){};
    virtual ~Downloader_impl(){curl_multi_cleanup(m_curl_multi_handle);}
};
    
class Action
{
protected:
    CURL *m_curl_handle;
    ConnectionInfo m_connection_info;
    long m_timeout;
    std::vector<uint8_t> m_response;
 
    /*!
     * Callback to process incoming data
     */
    static size_t write_save(void *buffer, size_t size, size_t nmemb,
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
        return 0;
    }
    
public:
    Action():
    m_curl_handle(curl_easy_init()),
    m_connection_info({"", 0, 0, 0, 0})
    {};
    virtual ~Action()
    {curl_easy_cleanup(m_curl_handle);};
    
    const std::vector<uint8_t>& getResponse(){return m_response;};
    
    bool perform()
    {
        CURLcode curlResult = curl_easy_perform(handle());
		return !curlResult;
    }
    
    CURL* handle() const {return m_curl_handle;}
    ConnectionInfo connection_info() const {return m_connection_info;}
    
    void set_handle(CURL *handle)
    {
        if(m_curl_handle)
        {
            curl_easy_cleanup(m_curl_handle);
        }
        
        m_curl_handle = handle;
    }
};

    
// Timeout interval for http requests
const long Downloader::DEFAULT_TIMEOUT = 1L;

class GetURLAction: public Action
{
private:
	string m_nodeUrl;
public:
	GetURLAction(const string &the_url) :
    Action(),
    m_nodeUrl(the_url)
	{
        m_connection_info.url = the_url;
        
        /* specify target URL, and note that this URL should include a file
		 name, not only a directory */
		curl_easy_setopt(handle(), CURLOPT_URL, m_nodeUrl.c_str());
		curl_easy_setopt(handle(), CURLOPT_WRITEDATA, this);
		curl_easy_setopt(handle(), CURLOPT_WRITEFUNCTION, write_save);
        curl_easy_setopt(handle(), CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(handle(), CURLOPT_PROGRESSDATA, this);
        curl_easy_setopt(handle(), CURLOPT_PROGRESSFUNCTION, progress_static);
	}
};

Downloader::Downloader() :
    m_impl(new Downloader_impl),
    m_maxQueueSize(50),
    m_timeout(DEFAULT_TIMEOUT),
    m_stop(false)
{

}
    
Downloader::~Downloader()
{
	stop();
}

std::vector<uint8_t> Downloader::getURL(const std::string &the_url)
{
	ActionPtr getNodeAction = make_shared<GetURLAction>(the_url);
    
    curl_easy_setopt(getNodeAction->handle(), CURLOPT_TIMEOUT, m_timeout);
	getNodeAction->perform();

	return getNodeAction->getResponse();
}
    
void Downloader::getURL_async(const std::string &the_url,
                              CompletionHandler ch,
                              ProgressHandler ph)
{
    // create an action which holds an easy handle
    ActionPtr getNodeAction = make_shared<GetURLAction>(the_url);
    
    // set options for this handle
    curl_easy_setopt(getNodeAction->handle(), CURLOPT_TIMEOUT, m_timeout);
    
    // add handle to multi
    curl_multi_add_handle(m_impl->m_curl_multi_handle, getNodeAction->handle());
}

void Downloader::addAction(const ActionPtr & theAction)
{
	std::unique_lock<std::mutex> lock(m_mutex);
    
	m_actionQueue.push_back(theAction);
    if(m_actionQueue.size() > m_maxQueueSize)
    {
        m_actionQueue.pop_front();
        //printf("dropping action...\n");
    }
	m_conditionVar.notify_one();
}

void Downloader::run()
{
	// init curl-handle for this thread
	CURL *handle = curl_easy_init();

	while (!m_stop)
	{
		ActionPtr action;
        
        curl_easy_reset(handle);

		// locked scope
		{
			std::unique_lock<std::mutex> lock(m_mutex);

			while (m_actionQueue.empty() && !m_stop)
				m_conditionVar.wait(lock);

			if (m_stop)
				break;

			action = m_actionQueue.front();
            curl_easy_setopt(action->handle(), CURLOPT_TIMEOUT, m_timeout);
			m_actionQueue.pop_front();
		}

		// go for gold
		action->perform();
	}

	//cleanup
	curl_easy_cleanup(handle);
}

long Downloader::getTimeOut()
{
    return m_timeout;
}
    
void Downloader::setTimeOut(long t)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_timeout=t;
}
    
}}// namespace