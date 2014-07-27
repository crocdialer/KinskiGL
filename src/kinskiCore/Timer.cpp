//
//  Timer.cpp
//  kinskiGL
//
//  Created by Croc Dialer on 27/07/14.
//
//

#include "Timer.h"
#include <boost/asio.hpp>
#include <chrono>

using std::chrono::duration_cast;
using std::chrono::steady_clock;

// ratio is 1 second per second, wow :D
typedef std::chrono::duration<float> float_second;

using namespace kinski;

struct Timer::timer_impl
{
    boost::asio::basic_waitable_timer<std::chrono::steady_clock> m_timer;
    Callback m_callback;
    
    timer_impl(boost::asio::io_service &io):
    m_timer(io)
    {}
};

Timer::Timer()
{

}

Timer::Timer(float secs, boost::asio::io_service &io, Callback cb):
m_impl(new timer_impl(io))
{
    expires_from_now(secs);
}

Timer::Timer(float secs, Callback cb)
{

}

void Timer::expires_from_now(float secs)
{
    m_impl->m_timer.expires_from_now(duration_cast<steady_clock::duration>(float_second(secs)));
    m_impl->m_timer.async_wait([&](const boost::system::error_code &error)
    {
        // Timer expired regularly
        if (!error)
        {
            if(m_impl->m_callback)
                m_impl->m_callback();
        }
    });
}

float Timer::expires_from_now() const
{
    auto duration = m_impl->m_timer.expires_from_now();
    return duration_cast<float_second>(duration).count();
}

void Timer::set_callback(Callback cb)
{
    if(m_impl)
    {
        m_impl->m_callback = cb;
    }
}
