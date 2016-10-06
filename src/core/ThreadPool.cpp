// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  ThreadPool.cpp
//
//  Created by Fabian on 6/12/13.

#include <boost/asio.hpp>
#include <thread>

#include "Timer.hpp"
#include "ThreadPool.hpp"

namespace kinski
{
    struct ThreadPool::Impl
    {
        boost::asio::io_service io_service;
        boost::asio::io_service::work io_work;
        std::vector<std::thread> threads;
        
        Impl():io_work(io_service){}
    };
    
    ThreadPool::ThreadPool(size_t num):
    m_impl(new Impl)
    {
        set_num_threads(num);
    }
    
    ThreadPool::~ThreadPool()
    {
        m_impl->io_service.stop();
        join_all();
    }
    
    void ThreadPool::submit(std::function<void()> the_task)
    {
        if(!the_task){ return; }
        m_impl->io_service.post(the_task);
    }
    
    void ThreadPool::submit_with_delay(std::function<void()> the_task, double the_delay)
    {
        if(!the_task){ return; }
        
        Timer t(m_impl->io_service);
        t.set_callback([t, the_task]()
        {
            the_task();
        });
        t.expires_from_now(the_delay);
    }
    
    boost::asio::io_service& ThreadPool::io_service()
    {
        return m_impl->io_service;
    }
    
    int ThreadPool::get_num_threads()
    {
        return m_impl->threads.size();
    }
    
    void ThreadPool::join_all()
    {
        for (auto &thread : m_impl->threads)
        {
            try
            {
                if(thread.joinable()){ thread.join(); }
            }
            catch(std::exception &e){LOG_ERROR<<e.what();}
        }
        m_impl->threads.clear();
    }
    
    void ThreadPool::set_num_threads(int num)
    {
        if(num >= 0)
        {
            try
            {
                join_all();
                
                for(int i = 0; i < num; ++i)
                {
                    m_impl->threads.push_back(std::thread([this](){ m_impl->io_service.run(); }));
                }
            }
            catch(std::exception &e){LOG_ERROR<<e.what();}
        }
    }
}
