//
//  Animation.h
//  kinskiGL
//
//  Created by Fabian on 8/17/13.
//
//

#ifndef __kinskiGL__Animation__
#define __kinskiGL__Animation__

#include <boost/function.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include "Property.h"
#include "Easing.h"

namespace kinski{ namespace animation{
    
    class Animation;
    typedef std::shared_ptr<Animation> AnimationPtr;
    typedef boost::function<float (float)> EaseFunction;
    typedef boost::function<void (float)> InterpolationFunction;
    typedef boost::function<void (void)> Callback;
    
    enum LoopType {LOOP_NONE = 0, LOOP = 1, LOOP_BACK_FORTH = 2};
    enum PlaybackType {PLAYBACK_PAUSED = 0, PLAYBACK_FORWARD = 1, PLAYBACK_BACKWARD = 2};
    
    class Animation
    {
    public:
        
        Animation(float duration, float delay, InterpolationFunction interpolate_fn):
        m_id(s_id_pool++),
        m_playing(PLAYBACK_FORWARD),
        m_loop_type(LOOP_NONE),
        m_start_time(boost::posix_time::second_clock::local_time()),
        m_end_time(m_start_time + boost::posix_time::seconds(duration)),
        m_current_time(m_start_time),
        m_ease_fn(EaseNone()),
        m_interpolate_fn(interpolate_fn){}
        
        int getId() const {return m_id;}
        inline float duration() const {return (m_end_time - m_start_time).total_nanoseconds() / 1.e9f;}
        inline bool playing() const {return m_playing != PLAYBACK_PAUSED;}
        inline void set_playing(PlaybackType playback_type = PLAYBACK_FORWARD){m_playing = playback_type;}
        inline LoopType loop() const {return m_loop_type;}
        inline void set_loop(LoopType loop_type = LOOP){m_loop_type = loop_type;}
        
        boost::posix_time::ptime start_time() const {return m_start_time;}
        boost::posix_time::ptime end_time() const {return m_end_time;}
        
        void set_ease_function(EaseFunction fn){m_ease_fn = fn;}
        void set_start_callback(Callback cb){m_start_fn = cb;}
        void set_update_callback(Callback cb){m_start_fn = cb;}
        void set_finish_callback(Callback cb){m_start_fn = cb;}
        void set_reverse_start_callback(Callback cb){m_start_fn = cb;}
        void set_reverse_finish_callback(Callback cb){m_start_fn = cb;}
        
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
            
            // this applies easing and sets an interpolated value
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
        
        inline void play(PlaybackType pt = PLAYBACK_FORWARD){ m_playing = pt; };
        
    private:
        
        static int s_id_pool;
        int m_id;
        PlaybackType m_playing;
        LoopType m_loop_type;
        boost::posix_time::ptime m_start_time, m_end_time, m_current_time;
        EaseFunction m_ease_fn;
        InterpolationFunction m_interpolate_fn;
        Callback m_start_fn, m_update_fn, m_finish_fn, m_reverse_start_fn, m_reverse_finish_fn;
    };
    
    int Animation::s_id_pool = 0;
    
//    template<typename T>
//    class InterpolationBase
//    {
//    public:
//        InterpolationBase(const T& from, const T& to):m_start_value(from), m_end_value(to){}
//        inline const T& start_val() const {return m_start_value;}
//        inline const T& end_val() const {return m_end_value;}
//    private:
//        T m_start_value, m_end_value;
//    };
    
    template<typename T>
    class PointerInterpolation
    {
    public:
        PointerInterpolation(T* the_ptr, const T& from, const T& to):
        m_value(the_ptr), m_start_value(from), m_end_value(to){}
        void operator()(float progress)
        {
            *m_value = mix(m_start_value, m_end_value, progress);
        }
    private:
        T* m_value;
        T m_start_value, m_end_value;
    };
    
    template<typename T>
    class PropertyInterpolation
    {
    public:
        PropertyInterpolation(typename Property_<T>::Ptr property, const T& from, const T& to):
        m_weak_property(property), m_start_value(from), m_end_value(to){}
        void operator()(float progress)
        {
            if(typename Property_<T>::Ptr prop = m_weak_property.lock())
            {
                *prop = mix(m_start_value, m_end_value, progress);
            }
        }
    private:
        typename std::weak_ptr<Property_<T> > m_weak_property;
        T m_start_value, m_end_value;
    };
    
    template<typename T>
    AnimationPtr createAnimation(T* value_ptr, const T &from_value, const T &to_value, float duration,
                                 float delay = 0)
    {
        PointerInterpolation<T> ptr_interpolate(value_ptr, from_value, to_value);
        return AnimationPtr(new Animation(duration, delay, ptr_interpolate));
    };
    
    template<typename T>
    AnimationPtr createAnimation(typename Property_<T>::Ptr property, const T &from_value, const T &to_value,
                                 float duration, float delay = 0)
    {
        PropertyInterpolation<T> prop_interpolate(property, from_value, to_value);
        return AnimationPtr(new Animation(duration, delay, prop_interpolate));
    };
    
}}//namespace

#endif /* defined(__kinskiGL__Animation__) */
