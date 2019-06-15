#include <utility>

#include <utility>

//
//  GOMTalking.cpp
//  MrProximity
//
//  Created by Fabian Schmidt on 11/20/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include <curl/curl.h>
#include <boost/asio.hpp>
#include <mutex>
#include "http.hpp"

using namespace std;
using duration_t = std::chrono::duration<double>;

namespace crocore {
namespace net {
namespace http {

using ActionPtr = std::shared_ptr<class CurlAction>;
using handle_map_t = std::map<CURL *, ActionPtr>;

// Timeout interval for http requests
static const long DEFAULT_TIMEOUT = 0;

///////////////////////////////////////////////////////////////////////////////

struct ClientImpl
{
    boost::asio::io_service *m_io_service;
    std::shared_ptr<CURLM> m_curl_multi_handle;

    // map containing current handles
    handle_map_t m_handle_map;

    // mutex to protect the handle-map
    std::mutex m_mutex;

    // connection timeout in ms
    uint64_t m_timeout;

    // number of running transfers
    int m_num_connections;

    explicit ClientImpl(boost::asio::io_service *io) :
            m_io_service(io),
            m_curl_multi_handle(curl_multi_init(), curl_multi_cleanup),
            m_timeout(DEFAULT_TIMEOUT),
            m_num_connections(0) {};

    void poll();

    void add_action(const ActionPtr &action, completion_cb_t ch, progress_cb_t ph = progress_cb_t());
};

///////////////////////////////////////////////////////////////////////////////

class CurlAction
{
private:
    std::shared_ptr<CURL> m_curl_handle;
    std::chrono::steady_clock::time_point m_start_time;
    completion_cb_t m_completion_handler;
    progress_cb_t m_progress_handler;
    response_t m_response;

    ///////////////////////////////////////////////////////////////////////////////

    /*!
     * callback to process incoming data
     */
    static size_t write_static(void *buffer, size_t size, size_t nmemb,
                               void *userp)
    {
        size_t num_bytes = size * nmemb;

        if(userp)
        {
            auto *ourAction = static_cast<CurlAction *>(userp);
            auto *buf_start = (uint8_t *)(buffer);
            uint8_t *buf_end = buf_start + num_bytes;
            ourAction->m_response.data.insert(ourAction->m_response.data.end(), buf_start, buf_end);
        }
        return num_bytes;
    }

    ///////////////////////////////////////////////////////////////////////////////

    /*!
     * callback for data provided to Curl for sending
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
     * callback to monitor transfer progress
     */
    static int progress_static(void *userp, double dltotal, double dlnow, double ult,
                               double uln)
    {
        auto *self = static_cast<CurlAction *>(userp);
        auto &con = self->m_response.connection;
        con.dl_total = dltotal;
        con.dl_now = dlnow;
        con.ul_total = ult;
        con.ul_now = uln;
        LOG_TRACE_2 << con.url << " : " << con.dl_now << " / " << con.dl_total;

        if(self->m_progress_handler){ self->m_progress_handler(con); }
        return 0;
    }

public:
    explicit CurlAction(const std::string &the_url) :
            m_curl_handle(curl_easy_init(), curl_easy_cleanup),
            m_start_time(std::chrono::steady_clock::now())
    {
        m_response.connection = {the_url, 0, 0, 0, 0, 0};
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

    response_t &response() { return m_response; };

    ///////////////////////////////////////////////////////////////////////////////

    bool perform()
    {
        CURLcode curlResult = curl_easy_perform(handle());
        curl_easy_getinfo(handle(), CURLINFO_RESPONSE_CODE, &m_response.status_code);
        if(!curlResult && m_completion_handler) m_completion_handler(m_response);
        return !curlResult;
    }

    ///////////////////////////////////////////////////////////////////////////////

    CURL *handle() const { return m_curl_handle.get(); }

    ///////////////////////////////////////////////////////////////////////////////

    connection_info_t connection_info() const { return m_response.connection; }

    ///////////////////////////////////////////////////////////////////////////////

    void set_handle(CURL *handle)
    {
        m_curl_handle = std::shared_ptr<CURL>(handle, curl_easy_cleanup);
    }

    ///////////////////////////////////////////////////////////////////////////////

    completion_cb_t completion_handler() const { return m_completion_handler; }

    void set_completion_handler(completion_cb_t ch) { m_completion_handler = std::move(ch); }

    void set_progress_handler(progress_cb_t ph) { m_progress_handler = std::move(ph); }

    ///////////////////////////////////////////////////////////////////////////////

    void set_timeout(uint64_t timeout)
    {
        m_response.connection.timeout = timeout;
        curl_easy_setopt(handle(), CURLOPT_TIMEOUT, timeout);
    }

    ///////////////////////////////////////////////////////////////////////////////

    double duration() const
    {
        return duration_t(std::chrono::steady_clock::now() - m_start_time).count();
    }
};

///////////////////////////////////////////////////////////////////////////////

typedef CurlAction Action_GET;

///////////////////////////////////////////////////////////////////////////////

class Action_POST : public CurlAction
{
private:
    std::vector<uint8_t> m_data;
    std::shared_ptr<struct curl_slist> m_headers;

public:
    Action_POST(const string &the_url,
                std::vector<uint8_t> the_data,
                const std::string &the_mime_type) :
            CurlAction(the_url),
            m_data(std::move(the_data))
    {
        auto header_content = "Content-Type: " + the_mime_type;
        m_headers = std::shared_ptr<struct curl_slist>(curl_slist_append(nullptr,
                                                                         header_content.c_str()),
                                                       curl_slist_free_all);

        curl_easy_setopt(handle(), CURLOPT_URL, the_url.c_str());
        curl_easy_setopt(handle(), CURLOPT_POSTFIELDS, &m_data[0]);
        curl_easy_setopt(handle(), CURLOPT_POSTFIELDSIZE, static_cast<curl_off_t>(m_data.size()));
        curl_easy_setopt(handle(), CURLOPT_HTTPHEADER, m_headers.get());
    }
};

///////////////////////////////////////////////////////////////////////////////

class Action_PUT : public CurlAction
{
private:
    std::vector<uint8_t> m_data;
    std::shared_ptr<struct curl_slist> m_headers;

public:
    Action_PUT(const string &the_url,
               std::vector<uint8_t> the_data,
               const std::string &the_mime_type) :
            CurlAction(the_url),
            m_data(std::move(the_data))
    {
        auto header_content = "Content-Type: " + the_mime_type;
        m_headers = std::shared_ptr<struct curl_slist>(curl_slist_append(nullptr,
                                                                         header_content.c_str()),
                                                       curl_slist_free_all);

        curl_easy_setopt(handle(), CURLOPT_URL, the_url.c_str());

        /* enable uploading */
        curl_easy_setopt(handle(), CURLOPT_UPLOAD, 1L);

        /* HTTP PUT please */
        curl_easy_setopt(handle(), CURLOPT_PUT, 1L);
        curl_easy_setopt(handle(), CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(m_data.size()));
        curl_easy_setopt(handle(), CURLOPT_HTTPHEADER, m_headers.get());
    }
};

///////////////////////////////////////////////////////////////////////////////

class Action_DELETE : public CurlAction
{
public:
    explicit Action_DELETE(const string &the_url) :
            CurlAction(the_url)
    {
        curl_easy_setopt(handle(), CURLOPT_CUSTOMREQUEST, "DELETE");
    }
};

///////////////////////////////////////////////////////////////////////////////

response_t head(const std::string &the_url)
{
    LOG_DEBUG << "head: '" << the_url << "'";
    ActionPtr url_action = make_shared<Action_GET>(the_url);
    curl_easy_setopt(url_action->handle(), CURLOPT_NOBODY, 1L);
    url_action->perform();
    return url_action->response();
}

///////////////////////////////////////////////////////////////////////////////

response_t get(const std::string &the_url)
{
    LOG_DEBUG << "get: '" << the_url << "'";
    ActionPtr url_action = make_shared<Action_GET>(the_url);
    url_action->perform();
    return url_action->response();
}

///////////////////////////////////////////////////////////////////////////////

response_t post(const std::string &the_url,
                const std::vector<uint8_t> &the_data,
                const std::string &the_mime_type)
{
    LOG_DEBUG << "post: '" << the_url << "'";
    ActionPtr url_action = make_shared<Action_POST>(the_url, the_data, the_mime_type);
    url_action->perform();
    return url_action->response();
}

///////////////////////////////////////////////////////////////////////////////

response_t put(const std::string &the_url,
               const std::vector<uint8_t> &the_data,
               const std::string &the_mime_type)
{
    LOG_DEBUG << "put: '" << the_url << "'";
    ActionPtr url_action = make_shared<Action_PUT>(the_url, the_data, the_mime_type);
    url_action->perform();
    return url_action->response();
}

///////////////////////////////////////////////////////////////////////////////

response_t del(const std::string &the_url)
{
    LOG_DEBUG << "delete: '" << the_url << "'";
    ActionPtr url_action = make_shared<Action_DELETE>(the_url);
    url_action->perform();
    return url_action->response();
}

///////////////////////////////////////////////////////////////////////////////

Client::Client(io_service_t &io) :
        m_impl(std::make_unique<ClientImpl>(&io))
{

}

///////////////////////////////////////////////////////////////////////////////

Client::Client(Client &&the_client) noexcept:
        m_impl(std::move(the_client.m_impl))
{

}

///////////////////////////////////////////////////////////////////////////////

Client::~Client() = default;

///////////////////////////////////////////////////////////////////////////////

Client &Client::operator=(Client other)
{
    std::swap(m_impl, other.m_impl);
    return *this;
}

///////////////////////////////////////////////////////////////////////////////

void ClientImpl::poll()
{
    curl_multi_perform(m_curl_multi_handle.get(), &m_num_connections);
    CURLMsg *msg;
    int msgs_left;

    while((msg = curl_multi_info_read(m_curl_multi_handle.get(), &msgs_left)))
    {
        if(msg->msg == CURLMSG_DONE)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            CURL *easy = msg->easy_handle;
            CURLcode res = msg->data.result;
            curl_multi_remove_handle(m_curl_multi_handle.get(), easy);
            auto itr = m_handle_map.find(easy);

            if(itr != m_handle_map.end())
            {
                auto &response = itr->second->response();
                response.duration = itr->second->duration();

                if(!res)
                {
                    // http response code
                    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &itr->second->response().status_code);

                    auto ci = itr->second->connection_info();
                    auto num_kb = static_cast<uint32_t >(ci.dl_total / 1024);

                    LOG_TRACE_1 << crocore::format("%d: '%s' completed successfully (%d kB, %d ms)",
                                                   response.status_code,
                                                   ci.url.c_str(),
                                                   num_kb,
                                                   (uint32_t)(response.duration * 1000));

                    if(itr->second->completion_handler())
                    {
                        itr->second->completion_handler()(response);
                    }
                }else
                {
                    LOG_DEBUG << "could not retrieve '" << itr->second->connection_info().url << "'";
                }
                m_handle_map.erase(itr);
            }
        }else{ LOG_DEBUG << msg->msg; }
    }

    if(m_num_connections && m_io_service)
    {
        m_io_service->post(std::bind(&ClientImpl::poll, this));
    }
}

///////////////////////////////////////////////////////////////////////////////

void ClientImpl::add_action(const ActionPtr &action, completion_cb_t ch, progress_cb_t ph)
{
    // set options for this handle
    action->set_timeout(m_timeout);
    action->set_completion_handler(std::move(ch));
    action->set_progress_handler(std::move(ph));

    std::unique_lock<std::mutex> lock(m_mutex);
    m_handle_map[action->handle()] = action;

    // add handle to multi
    curl_multi_add_handle(m_curl_multi_handle.get(), action->handle());

    if(m_io_service && !m_num_connections)
    {
        m_io_service->post(std::bind(&ClientImpl::poll, this));
    }
}

///////////////////////////////////////////////////////////////////////////////

void Client::async_head(const std::string &the_url,
                        completion_cb_t ch,
                        progress_cb_t ph)
{
    LOG_DEBUG << "async_head: '" << the_url << "'";
    ActionPtr url_action = make_shared<Action_GET>(the_url);
    curl_easy_setopt(url_action->handle(), CURLOPT_NOBODY, 1L);
    m_impl->add_action(url_action, std::move(ch), std::move(ph));
}

///////////////////////////////////////////////////////////////////////////////

void Client::async_get(const std::string &the_url,
                       completion_cb_t ch,
                       progress_cb_t ph)
{
    LOG_DEBUG << "async_get: '" << the_url << "'";

    // create an action which holds an easy handle
    ActionPtr url_action = make_shared<Action_GET>(the_url);
    m_impl->add_action(url_action, std::move(ch), std::move(ph));
}

///////////////////////////////////////////////////////////////////////////////

void Client::async_post(const std::string &the_url,
                        const std::vector<uint8_t> &the_data,
                        completion_cb_t ch,
                        const std::string &the_mime_type,
                        progress_cb_t ph)
{
    LOG_DEBUG << "async_post: '" << the_url << "'";
    ActionPtr url_action = make_shared<Action_POST>(the_url, the_data, the_mime_type);
    m_impl->add_action(url_action, std::move(ch), std::move(ph));
}

///////////////////////////////////////////////////////////////////////////////

void Client::async_put(const std::string &the_url,
                       const std::vector<uint8_t> &the_data,
                       completion_cb_t ch,
                       const std::string &the_mime_type,
                       progress_cb_t ph)
{
    LOG_DEBUG << "async_put: '" << the_url << "'";
    ActionPtr url_action = make_shared<Action_PUT>(the_url, the_data, the_mime_type);
    m_impl->add_action(url_action, std::move(ch), std::move(ph));
}

///////////////////////////////////////////////////////////////////////////////

void Client::async_del(const std::string &the_url, completion_cb_t ch)
{
    LOG_DEBUG << "async_del: '" << the_url << "'";
    ActionPtr url_action = make_shared<Action_DELETE>(the_url);
    m_impl->add_action(url_action, std::move(ch));
}

///////////////////////////////////////////////////////////////////////////////

uint64_t Client::timeout() const
{
    return m_impl->m_timeout;
}

///////////////////////////////////////////////////////////////////////////////

void Client::set_timeout(uint64_t t)
{
    m_impl->m_timeout = t;
}

///////////////////////////////////////////////////////////////////////////////

}
}
}// namespace
