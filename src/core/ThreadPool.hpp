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

#include <atomic>
#include <chrono>
#include "core/core.hpp"

namespace kinski
{

DEFINE_CLASS_PTR(Task);

class Task
{
public:
    static TaskPtr create(const std::string &the_desc = "",
                          std::function<void()> the_functor = std::function<void()>())
    {
        auto task = TaskPtr(new Task());
        if(the_functor){ task->add_work(the_functor); }
        task->set_description(the_desc);
        return task;
    }

    static uint32_t num_tasks()
    { return s_num_tasks; };

    ~Task()
    {
        auto task_str = m_description.empty() ? "task #" + to_string(m_id) : m_description;
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - m_start_time).count();
        LOG_DEBUG << "'" << task_str << "' completed (" << millis << " ms)";
        s_num_tasks--;
    }

    uint32_t id() const
    { return m_id; }

    void set_description(const std::string &the_desc){ m_description = the_desc; }
    std::string description() const { return m_description; }

    void add_work(std::function<void()> the_work){ m_functors.push_back(the_work); }

private:
    Task(): m_id(s_id_counter++), m_start_time(std::chrono::steady_clock::now())
    {
        s_num_tasks++;
    }

    static std::atomic<uint32_t> s_num_tasks;
    static std::atomic<uint32_t> s_id_counter;
    uint32_t m_id;
    std::chrono::steady_clock::time_point m_start_time;
    std::string m_description;
    std::vector<std::function<void()>> m_functors;
};

class ThreadPool
{
public:

    explicit ThreadPool(size_t num = 1);
    ThreadPool(ThreadPool &&other);
    ~ThreadPool();
    ThreadPool& operator=(ThreadPool other);

    void set_num_threads(int num);
    int get_num_threads();
    io_service_t& io_service();

    /*!
     * submit a task to be processed by the threadpool
     */
    KINSKI_API void submit(std::function<void()> the_task);

    /*!
     * submit a task to be processed by the threadpool
     * with an delay in seconds
     */
    KINSKI_API void submit_with_delay(std::function<void()> the_task, double the_delay);

    /*!
     * poll
     */
    KINSKI_API std::size_t poll();

    /*!
     * join_all
     */
    KINSKI_API void join_all();

private:
    std::unique_ptr<struct ThreadPoolImpl> m_impl;
};
}//namespace
