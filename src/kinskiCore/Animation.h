//
//  Animation.h
//  kinskiGL
//
//  Created by Fabian on 8/17/13.
//
//

#ifndef __kinskiGL__Animation__
#define __kinskiGL__Animation__

#include "boost/date_time/posix_time/posix_time.hpp"
#include "Property.h"
#include "Easing.h"

namespace kinski{ namespace animation{
    
    class Animation;
    typedef std::shared_ptr<Animation> AnimationPtr;
    typedef std::function<float (float)> EaseFunction;
    typedef std::function<void (float)> InterpolationFunction;
    typedef std::function<void (void)> Callback;
    
    enum LoopType {LOOP_NONE = 0, LOOP = 1, LOOP_BACK_FORTH = 2};
    enum PlaybackType {PLAYBACK_PAUSED = 0, PLAYBACK_FORWARD = 1, PLAYBACK_BACKWARD = 2};
    
    class Animation
    {
    public:
        
        Animation(float duration, float delay, InterpolationFunction interpolate_fn):
        m_playing(PLAYBACK_PAUSED),
        m_loop_type(LOOP_NONE),
        m_start_time(boost::posix_time::second_clock::local_time()),
        m_end_time(m_start_time + boost::posix_time::seconds(duration)),
        m_current_time(m_start_time),
        m_ease_fn(EaseNone()),
        m_interpolate_fn(interpolate_fn){}
        
        int getId() const {return m_id;}
        
        inline float duration() const {return (m_end_time - m_start_time).total_nanoseconds() / 1.e9f;}
        inline void set_duration(float d){ m_end_time = m_start_time + boost::posix_time::seconds(d); };
        
        inline PlaybackType playing() const {return m_playing;}
        inline void set_playing(PlaybackType playback_type = PLAYBACK_FORWARD){m_playing = playback_type;}
        
        inline LoopType loop() const {return m_loop_type;}
        inline void set_loop(LoopType loop_type = LOOP){m_loop_type = loop_type;}
        
        boost::posix_time::ptime start_time() const {return m_start_time;}
        boost::posix_time::ptime end_time() const {return m_end_time;}
        
        void set_ease_function(EaseFunction fn){m_ease_fn = fn;}
        void set_start_callback(Callback cb){m_start_fn = cb;}
        void set_update_callback(Callback cb){m_update_fn = cb;}
        void set_finish_callback(Callback cb){m_finish_fn = cb;}
        void set_reverse_start_callback(Callback cb){m_reverse_start_fn = cb;}
        void set_reverse_finish_callback(Callback cb){m_reverse_finish_fn = cb;}
        
        inline float progress() const
        {
            float val = clamp((float)(m_current_time - m_start_time).total_nanoseconds() /
                              (float)(m_end_time - m_start_time).total_nanoseconds(), 0.f, 1.f);
            
            if(m_playing == PLAYBACK_BACKWARD){val = 1.f - val;}
            return val;
        }
        
        inline bool finished() const
        {
            return m_current_time > m_end_time;
        }
        
        void update(float timeDelta)
        {
            if(!playing()) return;

            if(finished())
            {
                // fire finish callback, if any
                if(m_playing == PLAYBACK_FORWARD && m_finish_fn)
                    m_finish_fn();
                else if(m_playing == PLAYBACK_BACKWARD && m_reverse_finish_fn)
                    m_reverse_finish_fn();
                
                if(loop())
                {
                    if(m_loop_type == LOOP_BACK_FORTH)
                    {
                        m_playing = m_playing == PLAYBACK_FORWARD ? PLAYBACK_BACKWARD : PLAYBACK_FORWARD;
                    }
                    start();
                }
            }
            // update timing
            m_current_time += boost::posix_time::microseconds(timeDelta * 1.e6f);
            
            // this applies easing and passes it to an interpolation function
            m_interpolate_fn(m_ease_fn(progress()));
            
            // fire update callback, if any
            if(m_update_fn)
                m_update_fn();
        
        };
        
        /*!
         * Start the animation with an optional delay in seconds
         */
        void start(float delay = 0.f)
        {
            if(!m_playing)
                m_playing = PLAYBACK_FORWARD;
            
            float dur = duration();
            m_current_time = boost::posix_time::second_clock::local_time();
            m_start_time = m_current_time + boost::posix_time::microseconds(delay * 1.e6f);
            m_end_time = m_start_time + boost::posix_time::microseconds(dur * 1.e6f);
            
            // fire start callback, if any
            if(m_playing == PLAYBACK_FORWARD && m_start_fn)
                m_start_fn();
            else if(m_playing == PLAYBACK_BACKWARD && m_reverse_start_fn)
                m_reverse_start_fn();
        };
        
        inline void stop(){ m_playing = PLAYBACK_PAUSED;}
        
    private:
        
        int m_id;
        PlaybackType m_playing;
        LoopType m_loop_type;
        boost::posix_time::ptime m_start_time, m_end_time, m_current_time;
        EaseFunction m_ease_fn;
        InterpolationFunction m_interpolate_fn;
        Callback m_start_fn, m_update_fn, m_finish_fn, m_reverse_start_fn, m_reverse_finish_fn;
    };
    
    class SequentialAnimation : public Animation
    {
    
    };
    
    class ParallelAnimation : public Animation
    {
        
    };
    
    template<typename T>
    AnimationPtr create(T* value_ptr, const T &from_value, const T &to_value, float duration,
                        float delay = 0)
    {
        return AnimationPtr(new Animation(duration, delay,
                                          [=](float progress)
                                          {*value_ptr = mix(from_value, to_value, progress);}));
    };
    
    template<typename T>
    AnimationPtr create(typename Property_<T>::WeakPtr weak_property,
                        const T &from_value,
                        const T &to_value,
                        float duration,
                        float delay = 0)
    {
        return AnimationPtr(new Animation(duration, delay,
                                          [=](float progress)
                                          {
                                              if(auto property = weak_property.lock())
                                                  *property = mix(from_value, to_value, progress);
                                          }));
    };
    
}}//namespace

#endif /* defined(__kinskiGL__Animation__) */
