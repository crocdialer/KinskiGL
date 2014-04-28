//
//  GOMTalking.h
//  MrProximity
//
//  Created by Fabian Schmidt on 11/20/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#ifndef kinski__Downloader_h_INCLUDED
#define kinski__Downloader_h_INCLUDED

#include "kinskiCore/networking.h"

namespace kinski{ namespace net{
    
    struct ConnectionInfo
    {
        std::string url;
        double dl_total, dl_now, ul_total, ul_now;
    };
    
    class Downloader
    {
    public:
        
        typedef std::shared_ptr<class Action> ActionPtr;
        
        static const long DEFAULT_TIMEOUT;
        
        typedef std::function<void(ConnectionInfo)> ProgressHandler;
        typedef std::function<void(ConnectionInfo, const std::vector<uint8_t>&)> CompletionHandler;
        
        Downloader();
        Downloader(boost::asio::io_service &io);
        virtual ~Downloader();
        
        /*!
         * Download the resource at the given url (blocking) 
         */
        std::vector<uint8_t> getURL(const std::string &the_url);
        
        /*!
         * Download the resource at the given url (nonblocking)
         */
        void async_getURL(const std::string &the_url,
                          CompletionHandler ch,
                          ProgressHandler ph = ProgressHandler());
        
        long getTimeOut();
        void setTimeOut(long t);

        void poll();
        
    private:
        
        std::shared_ptr<struct Downloader_impl> m_impl;
        
        // connection timeout in ms
        long m_timeout;
        
        // number of running transfers
        int m_running;
    };
    
}}// namespace
#endif
