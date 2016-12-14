//
//  GOMTalking.h
//  MrProximity
//
//  Created by Fabian Schmidt on 11/20/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#pragma once

#include "core/networking.hpp"

namespace kinski{ namespace net{
    
    class Downloader
    {
    public:
        
        struct ConnectionInfo
        {
            std::string url;
            double dl_total, dl_now, ul_total, ul_now;
        };
        typedef std::function<void(ConnectionInfo)> progress_cb_t;
        typedef std::function<void(ConnectionInfo, const std::vector<uint8_t>&)> completion_cb_t;
        
        
        Downloader();
        Downloader(boost::asio::io_service &io);
        virtual ~Downloader();
        
        /*!
         * Download the resource at the given url (blocking) 
         */
        std::vector<uint8_t> get_url(const std::string &the_url);
        
        /*!
         * Download the resource at the given url (nonblocking)
         */
        void async_get_url(const std::string &the_url,
                           completion_cb_t ch = completion_cb_t(),
                           progress_cb_t ph = progress_cb_t());
        
        long timeout() const;
        void set_timeout(long t);

        void poll();
        
        void set_io_service(boost::asio::io_service &io);
        
    private:
        
        std::shared_ptr<struct DownloaderImpl> m_impl;
    };
    
}}// namespace
