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

///////////////////////////////////////////////////////////////////////////////

std::atomic<uint64_t> Task::s_id_counter(0);
std::mutex Task::s_mutex;
std::map<uint64_t, TaskWeakPtr> Task::s_tasks;

using std::chrono::duration_cast;
using std::chrono::steady_clock;

// 1 double per second
using duration_t = std::chrono::duration<double>;

double Task::duration() const
{
    return duration_cast<duration_t>(steady_clock::now() - m_start_time).count();
}

std::vector<TaskPtr> Task::current_tasks()
{
    std::vector<TaskPtr> ret(s_tasks.size());
    std::unique_lock<std::mutex> scoped_lock(s_mutex);
    size_t i = 0;

    for(const auto &p : s_tasks){ ret[i++] = p.second.lock(); }
    return ret;
}

///////////////////////////////////////////////////////////////////////////////

using io_work_ptr = std::unique_ptr<boost::asio::io_service::work>;

struct ThreadPoolImpl
{
    boost::asio::io_service io_service;
    io_work_ptr io_work;
    std::vector<std::thread> threads;
};

ThreadPool::ThreadPool(size_t num):
m_impl(std::make_unique<ThreadPoolImpl>())
{
    set_num_threads(num);
}

ThreadPool::ThreadPool(ThreadPool &&other)
{
    m_impl = std::move(other.m_impl);
}

ThreadPool::~ThreadPool()
{
    m_impl->io_service.stop();
    join_all();
}

ThreadPool& ThreadPool::operator=(ThreadPool other)
{
    std::swap(m_impl, other.m_impl);
    return *this;
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

std::size_t ThreadPool::poll()
{
    return m_impl->io_service.poll();
}

io_service_t& ThreadPool::io_service()
{
    return m_impl->io_service;
}

int ThreadPool::get_num_threads()
{
    return m_impl->threads.size();
}

void ThreadPool::join_all()
{
    m_impl->io_work.reset();

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
            m_impl->io_work = io_work_ptr(new boost::asio::io_service::work(m_impl->io_service));

            for(int i = 0; i < num; ++i)
            {
                m_impl->threads.push_back(std::thread([this](){ m_impl->io_service.run(); }));
            }
        }
        catch(std::exception &e){LOG_ERROR<<e.what();}
    }
}
}
