//
//  ThreadPool.h
//  kinskiGL
//
//  Created by Fabian on 6/12/13.
//
//

#ifndef __kinskiGL__ThreadPool__
#define __kinskiGL__ThreadPool__

#include "kinskiCore/Definitions.h"

// forward declared io_service
namespace boost
{
    namespace asio
    {
        class io_service;
    }
}

namespace kinski
{
    
    class ThreadPool
    {
    public:
        
        typedef std::function<void()> Task;
        
        ThreadPool(size_t num = 1);
        ~ThreadPool();
        
        void set_num_threads(int num);
        int get_num_threads();
        boost::asio::io_service& io_service();
        
        /*!
         * submit an arbitrary task to be processed by the threadpool
         */
        void submit(Task t);
        
    private:
        void join_all();
        std::unique_ptr<struct ThreadPoolImpl> m_impl;
    };
    
}//namespace

#endif /* defined(__kinskiGL__ThreadPool__) */
