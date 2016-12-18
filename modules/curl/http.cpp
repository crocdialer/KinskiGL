//
//  GOMTalking.cpp
//  MrProximity
//
//  Created by Fabian Schmidt on 11/20/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include "curl/curl.h"
#include <boost/asio.hpp>
#include "http.h"

using namespace std;

namespace kinski{ namespace net{ namespace http{

typedef std::shared_ptr<class Action> ActionPtr;
typedef std::map<CURL*, ActionPtr> HandleMap;
    
// Timeout interval for http requests
static const long DEFAULT_TIMEOUT = 0;

///////////////////////////////////////////////////////////////////////////////
    
struct ClientImpl
{
    boost::asio::io_service *m_io_service;
    std::shared_ptr<CURLM> m_curl_multi_handle;
    HandleMap m_handle_map;
    
    // connection timeout in ms
    uint64_t m_timeout;
    
    // number of running transfers
    int m_num_connections;
    
    ClientImpl(boost::asio::io_service *io):
    m_io_service(io),
    m_curl_multi_handle(curl_multi_init(), curl_multi_cleanup),
    m_timeout(DEFAULT_TIMEOUT),
    m_num_connections(0){};
    
    void poll();
};
    
///////////////////////////////////////////////////////////////////////////////
    
class Action
{
private:
    std::shared_ptr<CURL> m_curl_handle;
    ConnectionInfo m_connection_info;
    completion_cb_t m_completion_handler;
    progress_cb_t m_progress_handler;
    std::vector<uint8_t> m_response;
 
    ///////////////////////////////////////////////////////////////////////////////
    
    /*!
     * Callback to process incoming data
     */
    static size_t write_static(void *buffer, size_t size, size_t nmemb,
                               void *userp)
    {
        size_t num_bytes = size * nmemb;
        
        if(userp)
        {
            Action *ourAction = static_cast<Action*>(userp);
            uint8_t* buf_start = (uint8_t*)(buffer);
            uint8_t* buf_end = buf_start + num_bytes;
            ourAction->m_response.insert(ourAction->m_response.end(), buf_start, buf_end);
        }
        return num_bytes;
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    /*!
     * Callback for data provided to Curl for sending
     */
    static size_t read_static(void *ptr, size_t size, size_t nmemb,
                              void *inStream)
    {
        size_t max_num_bytes = size * nmemb;
//        size_t num_bytes = std::min(strlen((char*) inStream), max_num_bytes);
        size_t num_bytes = max_num_bytes;
        memcpy(ptr, inStream, num_bytes);
        return num_bytes;
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
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
        LOG_TRACE_2 << self->connection_info().url << " : " << self->connection_info().dl_now << " / "
        << self->connection_info().dl_total;
        
        if(self->m_progress_handler)
            self->m_progress_handler(self->m_connection_info);
        
        return 0;
    }
    
public:
    Action(const std::string &the_url):
    m_connection_info({the_url, 0, 0, 0, 0, 0})
    {
        set_handle(curl_easy_init());
        curl_easy_setopt(handle(), CURLOPT_WRITEDATA, this);
		curl_easy_setopt(handle(), CURLOPT_WRITEFUNCTION, write_static);
        curl_easy_setopt(handle(), CURLOPT_READDATA, this);
        curl_easy_setopt(handle(), CURLOPT_READFUNCTION, read_static);
        curl_easy_setopt(handle(), CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(handle(), CURLOPT_PROGRESSDATA, this);
        curl_easy_setopt(handle(), CURLOPT_PROGRESSFUNCTION, progress_static);
        curl_easy_setopt(handle(), CURLOPT_URL, the_url.c_str());
    };
    
    ///////////////////////////////////////////////////////////////////////////////
    
    const std::vector<uint8_t>& response(){return m_response;};
    
    ///////////////////////////////////////////////////////////////////////////////
    
    bool perform()
    {
        CURLcode curlResult = curl_easy_perform(handle());
        if(!curlResult && m_completion_handler) m_completion_handler(m_connection_info, m_response);
		return !curlResult;
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    CURL* handle() const {return m_curl_handle.get();}
    
    ///////////////////////////////////////////////////////////////////////////////
    
    ConnectionInfo connection_info() const {return m_connection_info;}
    
    ///////////////////////////////////////////////////////////////////////////////
    
    void set_handle(CURL *handle)
    {
        m_curl_handle = std::shared_ptr<CURL>(handle, curl_easy_cleanup);
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    
    completion_cb_t completion_handler() const {return m_completion_handler;}
    void set_completion_handler(completion_cb_t ch){m_completion_handler = ch;}
    void set_progress_handler(progress_cb_t ph){m_progress_handler = ph;}
    
    ///////////////////////////////////////////////////////////////////////////////
    
    void set_timeout(uint64_t the_timeout)
    {
        m_connection_info.timeout = the_timeout;
        curl_easy_setopt(handle(), CURLOPT_TIMEOUT, the_timeout);
    }
    
    ///////////////////////////////////////////////////////////////////////////////
};

///////////////////////////////////////////////////////////////////////////////
    
typedef Action Action_GET;
 
///////////////////////////////////////////////////////////////////////////////
    
class Action_POST: public Action
{
private:
    std::vector<uint8_t> m_data;
    std::shared_ptr<struct curl_slist> m_headers;
    
public:
    Action_POST(const string &the_url,
                const std::vector<uint8_t> &the_data,
                const std::string &the_mime_type = "text/json"):
    Action(the_url),
    m_data(the_data)
    {
        auto header_content = "Content-Type: " + the_mime_type;
        m_headers = std::shared_ptr<struct curl_slist>(curl_slist_append(nullptr,
                                                                         header_content.c_str()),
                                                       curl_slist_free_all);
        
        curl_easy_setopt(handle(), CURLOPT_URL, the_url.c_str());
        curl_easy_setopt(handle(), CURLOPT_POSTFIELDS, &m_data[0]);
        curl_easy_setopt(handle(), CURLOPT_POSTFIELDSIZE, m_data.size());
        curl_easy_setopt(handle(), CURLOPT_HTTPHEADER, m_headers.get());
    }
};

///////////////////////////////////////////////////////////////////////////////
    
std::vector<uint8_t> get(const std::string &the_url)
{
    LOG_DEBUG << "get: '" << the_url << "'";
    ActionPtr url_action = make_shared<Action_GET>(the_url);
    url_action->perform();
    return url_action->response();
}

///////////////////////////////////////////////////////////////////////////////
    
std::vector<uint8_t> post(const std::string &the_url,
                          const std::vector<uint8_t> &the_data,
                          const std::string &the_mime_type)
{
    LOG_DEBUG << "post: '" << the_url << "'";
    ActionPtr url_action = make_shared<Action_POST>(the_url, the_data, the_mime_type);
    url_action->perform();
    return url_action->response();
}

///////////////////////////////////////////////////////////////////////////////
    
Client::Client(boost::asio::io_service &io):
m_impl(new ClientImpl(&io))
{

}

///////////////////////////////////////////////////////////////////////////////
    
Client::~Client()
{

}
    
///////////////////////////////////////////////////////////////////////////////
    
void ClientImpl::poll()
{
    curl_multi_perform(m_curl_multi_handle.get(), &m_num_connections);
    CURLMsg *msg;
    int msgs_left;
    CURL *easy;
    CURLcode res;
    
    while((msg = curl_multi_info_read(m_curl_multi_handle.get(), &msgs_left)))
    {
        if(msg->msg == CURLMSG_DONE)
        {
            easy = msg->easy_handle;
            res = msg->data.result;
            
            curl_multi_remove_handle(m_curl_multi_handle.get(), easy);
            
            auto itr = m_handle_map.find(easy);
            if(itr != m_handle_map.end())
            {
                if(!res)
                {
                    auto ci = itr->second->connection_info();
                    int num_kb = ci.dl_total / 1024;
                    
                    LOG_DEBUG << "'" << ci.url << "' completed successfully (" << num_kb << " kB)";
                    
                    if(itr->second->completion_handler())
                    {
                        itr->second->completion_handler()(itr->second->connection_info(),
                                                          itr->second->response());
                    }
                }
                else
                {
                    LOG_DEBUG << "could not retrieve '" << itr->second->connection_info().url << "'";
                }
                m_handle_map.erase(itr);
            }
        }
        else{ LOG_DEBUG << msg->msg; }
    }
    
    if(m_num_connections && m_io_service)
    {
        m_io_service->post(std::bind(&ClientImpl::poll, this));
    }
}
    
///////////////////////////////////////////////////////////////////////////////
    
void Client::async_get(const std::string &the_url,
                       completion_cb_t ch,
                       progress_cb_t ph)
{
    LOG_DEBUG << "async_get: '" << the_url << "'";
    
    // create an action which holds an easy handle
    ActionPtr url_action = make_shared<Action_GET>(the_url);
    
    // set options for this handle
    url_action->set_timeout(m_impl->m_timeout);
    url_action->set_completion_handler(ch);
    url_action->set_progress_handler(ph);
    m_impl->m_handle_map[url_action->handle()] = url_action;
    
    // add handle to multi
    curl_multi_add_handle(m_impl->m_curl_multi_handle.get(), url_action->handle());
    
    if(m_impl->m_io_service && !m_impl->m_num_connections)
    {
        m_impl->m_io_service->post(std::bind(&ClientImpl::poll, m_impl));
    }
}
    
///////////////////////////////////////////////////////////////////////////////
    
void Client::async_post(const std::string &the_url,
                        const std::vector<uint8_t> &the_data,
                        completion_cb_t ch,
                        const std::string &the_mime_type,
                        progress_cb_t ph)
{
    LOG_DEBUG << "async_post: '" << the_url << "'";
    
    // create an action which holds an easy handle
    ActionPtr url_action = make_shared<Action_POST>(the_url, the_data, the_mime_type);
    
    // set options for this handle
    url_action->set_timeout(m_impl->m_timeout);
    url_action->set_completion_handler(ch);
    url_action->set_progress_handler(ph);
    m_impl->m_handle_map[url_action->handle()] = url_action;
    
    // add handle to multi
    curl_multi_add_handle(m_impl->m_curl_multi_handle.get(), url_action->handle());
    
    if(m_impl->m_io_service && !m_impl->m_num_connections)
    {
        m_impl->m_io_service->post(std::bind(&ClientImpl::poll, m_impl));
    }
}

///////////////////////////////////////////////////////////////////////////////
    
long Client::timeout() const
{
    return m_impl->m_timeout;
}
    
///////////////////////////////////////////////////////////////////////////////
    
void Client::set_timeout(long t)
{
    m_impl->m_timeout = t;
}

///////////////////////////////////////////////////////////////////////////////
    
}}}// namespace
