//
//  http.h
//  Kinski Framework
//
//  Created by Fabian Schmidt on 11/20/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#pragma once

#include <chrono>
#include "crocore/networking.hpp"

namespace crocore{ namespace net{ namespace http{
    
struct connection_info_t
{
    std::string url;
    double dl_total = 0, dl_now = 0, ul_total = 0, ul_now = 0;
    uint64_t timeout = 0;
};
    
struct response_t
{
    connection_info_t connection;
    uint64_t status_code = 0;
    std::vector<uint8_t> data;
    double duration = 0.0;
};
    
using progress_cb_t = std::function<void(connection_info_t)>;
using completion_cb_t = std::function<void(response_t&)>;

/*!
 * get the resource at the given url (blocking) with HTTP HEAD
 */
response_t head(const std::string &the_url);
    
/*!
 * get the resource at the given url (blocking) with HTTP GET
 */
response_t get(const std::string &the_url);

/*!
 * get the resource at the given url (blocking) with HTTP POST.
 * transmits <the_data> with the provided MIME-type.
 */
response_t post(const std::string &the_url,
                const std::vector<uint8_t> &the_data,
                const std::string &the_mime_type = "application/json");
    
/*!
 * upload <the_data> at the given url (blocking) with HTTP PUT.
 * transmits <the_data> and sets the Content-Type attribute to <the_mime_type>.
 */
response_t put(const std::string &the_url,
               const std::vector<uint8_t> &the_data,
               const std::string &the_mime_type = "application/json");

/*!
 * http DELETE
 */
response_t del(const std::string &the_url);

class Client
{
public:
    
    explicit Client(io_service_t &io);

    Client(Client &&the_client) noexcept;

    ~Client();

    Client& operator=(Client other);

    /*!
     * get the resource at the given url (non-blocking) with HTTP HEAD
     */
    void async_head(const std::string &the_url,
                    completion_cb_t ch = completion_cb_t(),
                    progress_cb_t ph = progress_cb_t());
    
    /*!
     * get the resource at the given url (non-blocking) with HTTP GET
     */
    void async_get(const std::string &the_url,
                   completion_cb_t ch = completion_cb_t(),
                   progress_cb_t ph = progress_cb_t());
    
    /*!
     * get the resource at the given url (non-blocking) with HTTP POST
     */
    void async_post(const std::string &the_url,
                    const std::vector<uint8_t> &the_data,
                    completion_cb_t ch = completion_cb_t(),
                    const std::string &the_mime_type = "application/json",
                    progress_cb_t ph = progress_cb_t());
    
    /*!
     * upload <the_data> at the given url (non-blocking) with HTTP PUT.
     * transmits <the_data> and sets the Content-Type attribute to <the_mime_type>.
     */
    void async_put(const std::string &the_url,
                   const std::vector<uint8_t> &the_data,
                   completion_cb_t ch = completion_cb_t(),
                   const std::string &the_mime_type = "application/json",
                   progress_cb_t ph = progress_cb_t());
    /*!
     * send an http DELETE request (non-blocking)
     */
    void async_del(const std::string &the_url, completion_cb_t ch = completion_cb_t());
    
    /*!
     * return the currently applied timeout for connections
     */
    uint64_t timeout() const;
    
    /*!
     * set the timeout for connections
     */
    void set_timeout(uint64_t t);

private:
    std::unique_ptr<struct ClientImpl> m_impl;
};
    
}}}// namespace
