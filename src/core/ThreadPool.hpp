//
//  ThreadPool.h
//  gl
//
//  Created by Fabian on 6/12/13.
//
//

#pragma once

#include "core/core.hpp"

namespace kinski
{
    
    class ThreadPool
    {
    public:
        
        ThreadPool(size_t num = 1);
        ~ThreadPool();
        
        void set_num_threads(int num);
        int get_num_threads();
        boost::asio::io_service& io_service();
        
        /*!
         * submit a task to be processed by the threadpool
         */
        void submit(std::function<void()> t);
        
    private:
        void join_all();
        
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
    
}//namespace