//
//  Animation.h
//  kinskiGL
//
//  Created by Fabian on 8/17/13.
//
//

#ifndef __kinskiGL__Animation__
#define __kinskiGL__Animation__

#include "Definitions.h"
#include "Logger.h"
#include "boost/date_time/posix_time/posix_time.hpp"

namespace kinski
{
    class Animation;
    typedef std::shared_ptr<Animation> AnimationPtr;
    
    class Animation
    {
    public:
        
        int getId() const {return m_id;}
        inline float duration() const {return (m_end_time - m_start_time).total_nanoseconds() / 1.e9f;}
        inline bool loop() const {return m_loop;}
        inline void set_loop(bool b = true){m_loop = b;}
        inline float progress() const
        {
            return clamp((float)(m_current_time - m_start_time).total_nanoseconds() /
            (float)(m_end_time - m_start_time).total_nanoseconds(), 0.f, 1.f);
        }
        inline bool finished() const {return m_current_time > m_end_time;}
        
        void update(float timeDelta)
        {
            m_current_time += boost::posix_time::microseconds(timeDelta * 1.e6f);
            update_internal(progress());
            if(finished())
            {
                if(loop()){ start(); }
                LOG_DEBUG<<"animation finished";
                return;
            }
        };
        
        void start()
        {
            float dur = duration();
            m_start_time = m_current_time = boost::posix_time::second_clock::local_time();
            m_end_time = m_start_time + boost::posix_time::seconds(dur);
        };
        
        void stop(){};
        
    private:
        
        virtual void update_internal(float progress) = 0;
        
        static int s_id_pool;
        int m_id;
        bool m_running;
        bool m_loop;
        boost::posix_time::ptime m_start_time, m_end_time, m_current_time;
        
    protected:
        Animation(float duration):
        m_id(s_id_pool++),
        m_running(true),
        m_loop(false),
        m_start_time(boost::posix_time::second_clock::local_time()),
        m_end_time(m_start_time + boost::posix_time::seconds(duration)),
        m_current_time(m_start_time){}
    };
    
    int Animation::s_id_pool = 0;
    
    template<typename T>
    class Animation_ : public Animation
    {
    public:
        
        static AnimationPtr create(T& value, const T &to_value, float duration)
        {
            return AnimationPtr(new Animation_<T>(value, to_value, duration));
        }
        
    private:
        
        Animation_(T& value, const T& to_value, float duration):
        Animation(duration),
        m_value(&value), m_start_value(value), m_end_value(to_value){}
        
        void update_internal(float progress)
        {
            *m_value = (1.f - progress) * m_start_value + progress * m_end_value;
        }
        
        T* m_value;
        T m_start_value, m_end_value;
    };
}//namespace

#endif /* defined(__kinskiGL__Animation__) */
