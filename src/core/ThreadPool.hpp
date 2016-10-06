// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//
//  ThreadPool.hpp
//
//  Created by Fabian on 6/12/13.

#pragma once

#include "core/core.hpp"

namespace kinski
{
    
    class ThreadPool
    {
    public:
        
        explicit ThreadPool(size_t num = 1);
        ~ThreadPool();
        
        void set_num_threads(int num);
        int get_num_threads();
        boost::asio::io_service& io_service();
        
        /*!
         * submit a task to be processed by the threadpool
         */
        KINSKI_API void submit(std::function<void()> the_task);
        
        /*!
         * submit a task to be processed by the threadpool
         * with an delay in seconds
         */
        KINSKI_API void submit_with_delay(std::function<void()> the_task, double the_delay);
        
    private:
        void join_all();
        
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
    
}//namespace
