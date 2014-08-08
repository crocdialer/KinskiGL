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
    Timer::Callback m_callback;
    bool periodic;
    
    timer_impl(boost::asio::io_service &io, Callback cb):
    m_timer(io),
    m_callback(cb),
    periodic(false)
    {}
};

Timer::Timer()
{

}

Timer::Timer(boost::asio::io_service &io, Callback cb):
m_impl(new timer_impl(io, cb)){}

Timer::Timer(float secs, boost::asio::io_service &io, Timer::Callback cb):
m_impl(new timer_impl(io, cb))
{
    expires_from_now(secs);
}

Timer::Timer(float secs, Timer::Callback cb)
{
    LOG_WARNING << "constructor not yet implemented";
}

void Timer::expires_from_now(float secs)
{
    if(!m_impl) return;
    
    m_impl->m_timer.expires_from_now(duration_cast<steady_clock::duration>(float_second(secs)));
    
    // make a tmp copy to solve obscure errors in lambda
    auto cb = m_impl->m_callback;
    
    m_impl->m_timer.async_wait([this, cb, secs](const boost::system::error_code &error)
    {
        // Timer expired regularly
        if (!error && cb) { cb(); }
        
        if(periodic()){ expires_from_now(secs); }
    });
}

float Timer::expires_from_now() const
{
    auto duration = m_impl->m_timer.expires_from_now();
    return duration_cast<float_second>(duration).count();
}

bool Timer::has_expired() const
{
    return expires_from_now() <= 0.f;
}

void Timer::cancel()
{
    if(m_impl)
        m_impl->m_timer.cancel();
}

bool Timer::periodic() const
{
    if(m_impl)
        return m_impl->periodic;
    
    return false;
}

void Timer::set_periodic(bool b)
{
    if(m_impl)
        m_impl->periodic = b;
}

void Timer::set_callback(Callback cb)
{
    if(m_impl)
    {
        m_impl->m_callback = cb;
    }
}
