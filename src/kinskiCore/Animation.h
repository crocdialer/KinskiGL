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
        
        Animation(float duration, float delay, InterpolationFunction interpolate_fn);
        
        int getId() const;
        
        virtual float duration() const;
        
        virtual void set_duration(float d);
        
        virtual PlaybackType playing() const;
        void set_playing(PlaybackType playback_type = PLAYBACK_FORWARD);
        
        LoopType loop() const;
        void set_loop(LoopType loop_type = LOOP);
        
        std::chrono::steady_clock::time_point start_time() const;
        std::chrono::steady_clock::time_point end_time() const;
        
        void set_interpolation_function(InterpolationFunction fn);
        void set_ease_function(EaseFunction fn);
        void set_start_callback(Callback cb);
        void set_update_callback(Callback cb);
        void set_finish_callback(Callback cb);
        void set_reverse_start_callback(Callback cb);
        void set_reverse_finish_callback(Callback cb);
        
        virtual float progress() const;
        
        virtual bool finished() const;
        
        virtual void update(float timeDelta);
        
        /*!
         * Start the animation with an optional delay in seconds
         */
        virtual void start(float delay = 0.f);
        
        virtual void stop();
        
    private:
        
        struct AnimationImpl;
        std::shared_ptr<AnimationImpl> m_impl;
        
    public:
        //! Emulates shared_ptr-like behavior
        typedef std::shared_ptr<AnimationImpl> Animation::*unspecified_bool_type;
        operator unspecified_bool_type() const { return ( m_impl.get() == 0 ) ? 0 : &Animation::m_impl; }
        void reset() { m_impl.reset(); }
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
