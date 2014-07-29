//
//  Animation.h
//  kinskiGL
//
//  Created by Fabian on 8/17/13.
//
//

#ifndef __kinskiGL__Animation__
#define __kinskiGL__Animation__

#include <chrono>
#include "Property.h"
#include "Easing.h"

namespace kinski{ namespace animation{
    
    using std::chrono::duration_cast;
    using std::chrono::microseconds;
    using std::chrono::steady_clock;
    
    // ratio is 1 second per second, wow :D
    typedef std::chrono::duration<float> float_second;
    
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
        m_start_time(steady_clock::now()),
        m_end_time(m_start_time + duration_cast<steady_clock::duration>(float_second(duration))),
        m_current_time(m_start_time),
        m_ease_fn(EaseNone()),
        m_interpolate_fn(interpolate_fn)
        {}
        
        int getId() const {return m_id;}
        
        virtual float duration() const
        {return duration_cast<float_second>(m_end_time - m_start_time).count();}
        
        virtual void set_duration(float d)
        {
            m_end_time = m_start_time + duration_cast<steady_clock::duration>(float_second(d));
        };
        
        virtual PlaybackType playing() const {return m_playing;}
        inline void set_playing(PlaybackType playback_type = PLAYBACK_FORWARD){m_playing = playback_type;}
        
        inline LoopType loop() const {return m_loop_type;}
        inline void set_loop(LoopType loop_type = LOOP){m_loop_type = loop_type;}
        
        steady_clock::time_point start_time() const {return m_start_time;}
        steady_clock::time_point end_time() const {return m_end_time;}
        
        void set_interpolation_function(InterpolationFunction fn){m_interpolate_fn = fn;}
        void set_ease_function(EaseFunction fn){m_ease_fn = fn;}
        void set_start_callback(Callback cb){m_start_fn = cb;}
        void set_update_callback(Callback cb){m_update_fn = cb;}
        void set_finish_callback(Callback cb){m_finish_fn = cb;}
        void set_reverse_start_callback(Callback cb){m_reverse_start_fn = cb;}
        void set_reverse_finish_callback(Callback cb){m_reverse_finish_fn = cb;}
        
        virtual float progress() const
        {
            float val = clamp(duration_cast<float_second>(m_current_time - m_start_time).count() /
                              duration_cast<float_second>(m_end_time - m_start_time).count(), 0.f, 1.f);
            
            if(m_playing == PLAYBACK_BACKWARD){val = 1.f - val;}
            return val;
        }
        
        virtual bool finished() const
        {
            return m_current_time > m_end_time;
        }
        
        virtual void update(float timeDelta)
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
                else
                {
                    // end playback
                    stop();
                }
            }
            // update timing
            m_current_time += duration_cast<steady_clock::duration>(float_second(timeDelta));
            
            if(m_current_time > m_start_time && m_current_time < m_end_time)
            {
                // this applies easing and passes it to an interpolation function
                m_interpolate_fn(m_ease_fn(progress()));
                
                // fire update callback, if any
                if(m_update_fn)
                    m_update_fn();
            }
        };
        
        /*!
         * Start the animation with an optional delay in seconds
         */
        virtual void start(float delay = 0.f)
        {
            if(!m_playing)
                m_playing = PLAYBACK_FORWARD;
            
            float dur = duration();
            m_current_time = steady_clock::now();
            m_start_time = m_current_time + duration_cast<steady_clock::duration>(float_second(delay));
            m_end_time = m_start_time + duration_cast<steady_clock::duration>(float_second(dur));
            
            // fire start callback, if any
            if(m_playing == PLAYBACK_FORWARD && m_start_fn)
                m_start_fn();
            else if(m_playing == PLAYBACK_BACKWARD && m_reverse_start_fn)
                m_reverse_start_fn();
        };
        
        virtual void stop(){ m_playing = PLAYBACK_PAUSED;}
        
    private:
        
        int m_id;
        PlaybackType m_playing;
        LoopType m_loop_type;
        steady_clock::time_point m_start_time, m_end_time, m_current_time;
        EaseFunction m_ease_fn;
        InterpolationFunction m_interpolate_fn;
        Callback m_start_fn, m_update_fn, m_finish_fn, m_reverse_start_fn, m_reverse_finish_fn;
    };
    
    class CompoundAnimation : public Animation
    {
    public:
        
        CompoundAnimation():Animation(0.f, 0.f, InterpolationFunction()){}
        
        virtual void start(float delay = 0.f)
        {
            for(const auto &child_anim : m_animations)
                child_anim->start();
        }
        virtual void stop()
        {
            for(const auto &child_anim : m_animations)
                child_anim->stop();
        }
        
        virtual float duration() const
        {
            if(m_animations.empty()) return 0.f;
            
            // find min start time and max end time
            steady_clock::time_point start_tp = steady_clock::time_point::max(),
            end_tp = steady_clock::time_point::min();
            
            for(const auto &child_anim : m_animations)
            {
                if(child_anim->start_time() < start_tp)
                    start_tp = child_anim->start_time();
                if(child_anim->end_time() > end_tp)
                    end_tp = child_anim->end_time();
                
            }
            return duration_cast<float_second>(end_tp - start_tp).count();
        }
        
        virtual void update(float timeDelta)
        {
            for(const auto &child_anim : m_animations){ child_anim->update(timeDelta); }
        }
        
        virtual bool finished() const
        {
            for(const auto &child_anim : m_animations)
            {
                if(!child_anim->finished())
                    return false;
            }
            return true;
        }
        
        std::vector<AnimationPtr>& children() {return m_animations;}
        const std::vector<AnimationPtr>& children() const {return m_animations;}
        
    private:
        std::vector<AnimationPtr> m_animations;
    };
    
//    class SequentialAnimation : public CompoundAnimation
//    {
//        virtual float duration() const;
//    };
//    
//    class ParallelAnimation : public CompoundAnimation
//    {
//        virtual float duration() const;
//    };
    
//    AnimationPtr create(float duration, float delay, InterpolationFunction interpolate_fn)
//    { return std::make_shared<Animation>(duration, delay, interpolate_fn); };
    
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
