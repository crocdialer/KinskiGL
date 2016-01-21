//
//  GOMTalking.h
//  MrProximity
//
//  Created by Fabian Schmidt on 11/20/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#ifndef kinski__Downloader_h_INCLUDED
#define kinski__Downloader_h_INCLUDED

#include "core/networking.h"

namespace kinski{ namespace net{
    
    class Downloader
    {
    public:
        
        struct ConnectionInfo
        {
            std::string url;
            double dl_total, dl_now, ul_total, ul_now;
        };
        typedef std::function<void(ConnectionInfo)> ProgressHandler;
        typedef std::function<void(ConnectionInfo, const std::vector<uint8_t>&)> CompletionHandler;
        
        
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
                           CompletionHandler ch = CompletionHandler(),
                           ProgressHandler ph = ProgressHandler());
        
        long timeout() const;
        void set_timeout(long t);

        void poll();
        
        void set_io_service(boost::asio::io_service &io);
        
    private:
        
        struct Impl;
        std::shared_ptr<Impl> m_impl;
    };
    
}}// namespace
#endif
