//
//  Timer.cpp
//  gl
//
//  Created by Croc Dialer on 27/07/14.
//
//

#include "Timer.hpp"
#include <boost/asio.hpp>
#include <chrono>

using std::chrono::duration_cast;
using std::chrono::steady_clock;

// ratio is 1 second per second, wow :D
typedef std::chrono::duration<float> float_second;

using namespace kinski;

struct Stopwatch::stopwatch_impl
{
    bool running;
    std::chrono::steady_clock::time_point start_time;
    std::vector<float> laps;
    
    stopwatch_impl():
    running(false),
    start_time(steady_clock::now()),
    laps({0.f}){}
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
    m_impl->laps.back() += duration_cast<float_second>(steady_clock::now() - m_impl->start_time).count();
}

bool Stopwatch::running()
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
    
    m_impl->laps.back() += duration_cast<float_second>(steady_clock::now() - m_impl->start_time).count();
    m_impl->start_time = steady_clock::now();
    m_impl->laps.push_back(0.f);
}

float Stopwatch::time_elapsed()
{
    float ret = 0.f;
    
    for(auto lap_time : m_impl->laps){ ret += lap_time; }
    
    if(!m_impl->running) return ret;
    
    ret += duration_cast<float_second>(steady_clock::now() - m_impl->start_time).count();
    return ret;
}

float Stopwatch::time_elapsed_for_lap()
{
    float ret = m_impl->laps.back();
    
    if(!m_impl->running) return ret;
    
    ret += duration_cast<float_second>(steady_clock::now() - m_impl->start_time).count();
    return ret;
}

const std::vector<float>& Stopwatch::laps()
{
    return m_impl->laps;
}

/////////////////////////////////////////////////////////////////////////

struct Timer::timer_impl
{
    boost::asio::basic_waitable_timer<std::chrono::steady_clock> m_timer;
    Timer::Callback m_callback;
    bool periodic;
    bool running;
    
    timer_impl(boost::asio::io_service &io, Callback cb):
    m_timer(io),
    m_callback(cb),
    periodic(false),
    running(false)
    {}
};

Timer::Timer()
{

}

Timer::Timer(boost::asio::io_service &io, Callback cb):
m_impl(new timer_impl(io, cb)){}

void Timer::expires_from_now(float secs)
{
    if(!m_impl) return;
    
    m_impl->m_timer.expires_from_now(duration_cast<steady_clock::duration>(float_second(secs)));
    m_impl->running = true;
    
    m_impl->m_timer.async_wait([this, secs](const boost::system::error_code &error)
    {
        m_impl->running = false;
        
        // Timer expired regularly
        if (!error)
        {
            if(m_impl->m_callback) { m_impl->m_callback(); }
            if(periodic()){ expires_from_now(secs); }
        }
    });
}

float Timer::expires_from_now() const
{
    auto duration = m_impl->m_timer.expires_from_now();
    return duration_cast<float_second>(duration).count();
}

bool Timer::has_expired() const
{
    return !m_impl->running;
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
