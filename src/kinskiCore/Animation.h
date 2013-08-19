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
#include "Property.h"
#include "Logger.h"
#include "boost/date_time/posix_time/posix_time.hpp"

namespace kinski
{
    class Animation;
    typedef std::shared_ptr<Animation> AnimationPtr;
    
    class Animation
    {
    public:
        
        enum LoopType {LOOP_NONE = 0, LOOP = 1, LOOP_BACK_FORTH = 2};
        enum PlaybackType {PLAYBACK_PAUSED = 0, PLAYBACK_FORWARD = 1, PLAYBACK_BACKWARD = 2};
        
        int getId() const {return m_id;}
        inline float duration() const {return (m_end_time - m_start_time).total_nanoseconds() / 1.e9f;}
        inline PlaybackType playing() const {return m_playing;}
        inline void set_playing(PlaybackType playback_type = PLAYBACK_FORWARD){m_playing = playback_type;}
        inline LoopType loop() const {return m_loop_type;}
        inline void set_loop(LoopType loop_type = LOOP){m_loop_type = loop_type;}
        
        inline float progress() const
        {
            float val = clamp((float)(m_current_time - m_start_time).total_nanoseconds() /
                              (float)(m_end_time - m_start_time).total_nanoseconds(), 0.f, 1.f);
            return val;
        }
        
        inline bool finished() const
        {
            return m_current_time > m_end_time || m_current_time < m_start_time;
        }
        
        void update(float timeDelta)
        {
            if(!m_playing) return;
            if(m_playing == PLAYBACK_BACKWARD)
                timeDelta *= -1.f;
            
            m_current_time += boost::posix_time::microseconds(timeDelta * 1.e6f);
            update_internal(progress());
            
            if(finished())
            {
                if(loop())
                {
                    if(m_loop_type == LOOP_BACK_FORTH)
                    {
                        m_playing = m_playing == PLAYBACK_FORWARD ? PLAYBACK_BACKWARD : PLAYBACK_FORWARD;
                    }
                    start();
                }
                LOG_DEBUG<<"animation finished";
            }
        };
        
        void start(float delay = 0.f)
        {
            if(!m_playing)
                m_playing = PLAYBACK_FORWARD;
            
            float dur = duration();
            m_start_time = boost::posix_time::second_clock::local_time()
                + boost::posix_time::microseconds(delay * 1.e6f);
            m_end_time = m_start_time + boost::posix_time::seconds(dur);
            m_current_time = m_playing == PLAYBACK_FORWARD ? m_start_time : m_end_time;
        };
        
        void stop(){m_playing = PLAYBACK_PAUSED;};
        
    private:
        
        virtual void update_internal(float progress) = 0;
        
        static int s_id_pool;
        int m_id;
        PlaybackType m_playing;
        LoopType m_loop_type;
        boost::posix_time::ptime m_start_time, m_end_time, m_current_time;
        
    protected:
        Animation(float duration):
        m_id(s_id_pool++),
        m_playing(PLAYBACK_FORWARD),
        m_loop_type(LOOP_NONE),
        m_start_time(boost::posix_time::second_clock::local_time()),
        m_end_time(m_start_time + boost::posix_time::seconds(duration)),
        m_current_time(m_start_time){}
    };
    
    int Animation::s_id_pool = 0;
    
    template<typename T>
    class Animation_ : public Animation
    {
    public:
        
        static AnimationPtr create(T* value_ptr, const T &to_value, float duration)
        {
            return AnimationPtr(new Animation_<T>(value_ptr, to_value, duration));
        }
        
        static AnimationPtr create(typename Property_<T>::Ptr property, const T &to_value, float duration)
        {
            T *value_ptr = property->template getValuePtr<T>();
            return AnimationPtr(new Animation_<T>(value_ptr, to_value, duration));
        }
        
    private:
        
        Animation_(T* value_ptr, const T& to_value, float duration):
        Animation(duration),
        m_value(value_ptr), m_start_value(*value_ptr), m_end_value(to_value){}
        
        void update_internal(float progress)
        {
            //TODO: animation curves / easing
            *m_value = (1.f - progress) * m_start_value + progress * m_end_value;
        }
        
        T* m_value;
        T m_start_value, m_end_value;
    };
}//namespace

#endif /* defined(__kinskiGL__Animation__) */
