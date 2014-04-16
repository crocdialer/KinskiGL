//
//  GOMTalking.h
//  MrProximity
//
//  Created by Fabian Schmidt on 11/20/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#ifndef kinski__GetURL_h_INCLUDED
#define kinski__GetURL_h_INCLUDED

#include "kinskiCore/Definitions.h"
#include <thread>
#include <mutex>
#include <condition_variable>


namespace kinski{ namespace net{
    
    class GetURL
    {
    public:
        
        static const long DEFAULT_TIMEOUT;
        
        GetURL();
        virtual ~GetURL();
        
        /*!
         * Download the resource at the given url (blocking) 
         */
        std::vector<uint8_t> getURL(const std::string &the_url);
        
        long getTimeOut();
        void setTimeOut(long t);
        
        void run();
        
    private:
        
        typedef std::shared_ptr<class Action> ActionPtr;
        
        void stop(){};
        void addAction(const ActionPtr& theAction);
        
        std::deque<ActionPtr> m_actionQueue;
        unsigned int m_maxQueueSize;
        
        long m_timeout;
        
        // threading
        volatile bool m_stop;
        std::thread m_thread;
        std::mutex m_mutex;
        std::condition_variable m_conditionVar;
    };
    
}}// namespace
#endif
