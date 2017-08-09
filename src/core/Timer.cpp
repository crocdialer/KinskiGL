// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  Timer.cpp
//
//  Created by Croc Dialer on 27/07/14.

#include "Timer.hpp"
#include <boost/asio.hpp>
#include <chrono>

using std::chrono::duration_cast;
using std::chrono::steady_clock;

// 1 double per second
typedef std::chrono::duration<double> duration_t;

namespace kinski
{

struct stopwatch_impl
{
    bool running;
    std::chrono::steady_clock::time_point start_time;
    std::vector<double> laps;
    
    stopwatch_impl():
    running(false),
    start_time(steady_clock::now()),
    laps({0.0}){}
};

Stopwatch::Stopwatch():m_impl(new stopwatch_impl)
{

}

void Stopwatch::start()
{
    if(m_impl->running) return;
    
    m_impl->running = true;
    m_impl->start_time = steady_clock::now();
}

void Stopwatch::stop()
{
    if(!m_impl->running) return;
    m_impl->running = false;
    m_impl->laps.back() += duration_cast<duration_t>(steady_clock::now() - m_impl->start_time).count();
}

bool Stopwatch::running() const
{
    return m_impl->running;
}

void Stopwatch::reset()
{
    m_impl.reset(new stopwatch_impl);
}

void Stopwatch::new_lap()
{
    if(!m_impl->running) return;
    
    m_impl->laps.back() += duration_cast<duration_t>(steady_clock::now() - m_impl->start_time).count();
    m_impl->start_time = steady_clock::now();
    m_impl->laps.push_back(0.0);
}

double Stopwatch::time_elapsed() const
{
    double ret = 0.0;
    
    for(auto lap_time : m_impl->laps){ ret += lap_time; }
    
    if(!m_impl->running) return ret;
    
    ret += duration_cast<duration_t>(steady_clock::now() - m_impl->start_time).count();
    return ret;
}

double Stopwatch::time_elapsed_for_lap() const
{
    if(m_impl->running)
    {
        return m_impl->laps.back() + duration_cast<duration_t>(steady_clock::now() -
                                                               m_impl->start_time).count();
    }
    else{ return m_impl->laps.back(); }
}

const std::vector<double>& Stopwatch::laps() const
{
    return m_impl->laps;
}

/////////////////////////////////////////////////////////////////////////

struct timer_impl
{
    boost::asio::basic_waitable_timer<std::chrono::steady_clock> m_timer;
    Timer::timer_cb_t m_callback;
    bool m_periodic;
    bool m_running;
    
    timer_impl(boost::asio::io_service &io, Timer::timer_cb_t cb):
    m_timer(io),
    m_callback(cb),
    m_periodic(false),
    m_running(false)
    {}
    
    ~timer_impl()
    {
        try{ m_timer.cancel(); }
        catch(boost::system::system_error& e){ LOG_WARNING << e.what(); }
    }
};

Timer::Timer(){}

Timer::Timer(io_service_t &io, Timer::timer_cb_t cb):
m_impl(new timer_impl(io, cb)){}

void Timer::expires_from_now(double secs)
{
    if(!m_impl) return;
    std::weak_ptr<timer_impl> weak_impl = m_impl;
    
    m_impl->m_timer.expires_from_now(duration_cast<steady_clock::duration>(duration_t(secs)));
    m_impl->m_running = true;

    m_impl->m_timer.async_wait([this, weak_impl, secs](const boost::system::error_code &error)
    {
        // Timer expired regularly
        if(!error)
        {
            auto impl = weak_impl.lock();
            
            if(impl)
            {
                impl->m_running = false;
                if(impl->m_callback) { impl->m_callback(); }
                if(impl->m_periodic){ expires_from_now(secs); }
            }
        }
    });
}

double Timer::expires_from_now() const
{
    if(!m_impl) return 0.0;
    auto duration = m_impl->m_timer.expires_from_now();
    return duration_cast<duration_t>(duration).count();
}

bool Timer::has_expired() const
{
    return !(m_impl && m_impl->m_running);
}

void Timer::cancel()
{
    if(m_impl){ m_impl->m_running = false; m_impl->m_timer.cancel(); }
}

bool Timer::periodic() const
{
    return m_impl && m_impl->m_periodic;
}

void Timer::set_periodic(bool b)
{
    if(m_impl){ m_impl->m_periodic = b; }
}

void Timer::set_callback(Timer::timer_cb_t cb)
{
    if(m_impl){ m_impl->m_callback = cb; }
}
}//namespace
