//
//  GOMTalking.h
//  MrProximity
//
//  Created by Fabian Schmidt on 11/20/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#pragma once

#include "core/networking.hpp"

namespace kinski{ namespace net{ namespace http{
    
typedef struct
{
    std::string url;
    double dl_total, dl_now, ul_total, ul_now;
    uint64_t timeout;
} connection_info_t;
    
typedef struct
{
    uint64_t status_code;
    std::vector<uint8_t> data;
} response_t;
    
typedef std::function<void(connection_info_t)> progress_cb_t;
typedef std::function<void(const connection_info_t&, const response_t&)> completion_cb_t;

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
 * transmits <the_data> and sets the Conten-Type attribute to <the_mime_type>.
 */
response_t put(const std::string &the_url,
               const std::vector<uint8_t> &the_data,
               const std::string &the_mime_type = "application/json");

/*!
 * http delete
 */
response_t del(const std::string &the_url);

class Client
{
public:
    
    Client(boost::asio::io_service &io);
    ~Client();
    
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
     * transmits <the_data> and sets the Conten-Type attribute to <the_mime_type>.
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
    
    uint64_t timeout() const;
    void set_timeout(uint64_t t);
    
private:
    std::shared_ptr<struct ClientImpl> m_impl;
};
    
}}}// namespace
