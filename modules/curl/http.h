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
    
struct ConnectionInfo
{
    std::string url;
    double dl_total, dl_now, ul_total, ul_now;
};
typedef std::function<void(ConnectionInfo)> progress_cb_t;
typedef std::function<void(ConnectionInfo, const std::vector<uint8_t>&)> completion_cb_t;

/*!
 * get the resource at the given url (blocking) with HTTP GET
 */
std::vector<uint8_t> get(const std::string &the_url);

/*!
 * get the resource at the given url (blocking) with HTTP POST.
 * transmits <the_data> with the provided MIME-type.
 */
std::vector<uint8_t> post(const std::string &the_url,
                          const std::vector<uint8_t> &the_data,
                          const std::string &the_mime_type = "text/json");

class Client
{
public:
    
    Client(boost::asio::io_service &io);
    ~Client();
    
    /*!
     * get the resource at the given url (nonblocking) with HTTP GET
     */
    void async_get(const std::string &the_url,
                   completion_cb_t ch = completion_cb_t(),
                   progress_cb_t ph = progress_cb_t());
    
    /*!
     * get the resource at the given url (nonblocking) with HTTP GET
     */
    void async_post(const std::string &the_url,
                    const std::vector<uint8_t> &the_data,
                    completion_cb_t ch = completion_cb_t(),
                    const std::string &the_mime_type = "text/json",
                    progress_cb_t ph = progress_cb_t());
    
    long timeout() const;
    void set_timeout(long t);
    
private:
    std::shared_ptr<struct ClientImpl> m_impl;
};
    
}}}// namespace
