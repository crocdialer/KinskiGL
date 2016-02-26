//
//  Animation.h
//  gl
//
//  Created by Fabian on 8/17/13.
//
//

#pragma once

#include <chrono>
#include "Property.hpp"
#include "Easing.hpp"

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
        
        Animation();
        Animation(float duration, float delay, InterpolationFunction interpolate_fn);
        virtual ~Animation();
        
        int getId() const;
        
        virtual float duration() const;
        
        virtual void set_duration(float d);
        
        virtual bool is_playing() const;
        virtual PlaybackType playbacktype() const;
        void set_playback_type(PlaybackType playback_type = PLAYBACK_FORWARD);
        
        LoopType loop() const;
        void set_loop(LoopType loop_type = LOOP);
        
//        std::chrono::steady_clock::time_point start_time() const;
//        std::chrono::steady_clock::time_point end_time() const;
        
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
    };
    
    class CompoundAnimation : public Animation
    {
    public:
        
        CompoundAnimation();
        
        virtual void start(float delay = 0.f);
        virtual void stop();
        
        virtual float duration() const;
        virtual void update(float timeDelta);
        virtual bool finished() const;
        
        std::vector<Animation>& children() {return m_animations;}
        const std::vector<Animation>& children() const {return m_animations;}
        
    private:
        std::vector<Animation> m_animations;
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
        return std::make_shared<Animation>(duration, delay, [=](float progress)
        {
            *value_ptr = kinski::mix(from_value, to_value, progress);
        });
    };
    
    template<typename T>
    AnimationPtr create(typename Property_<T>::WeakPtr weak_property,
                        const T &from_value,
                        const T &to_value,
                        float duration,
                        float delay = 0)
    {
        return std::make_shared<Animation>(duration, delay, [=](float progress)
        {
            if(auto property = weak_property.lock())
            {
                *property = kinski::mix(from_value, to_value, progress);
            }
        });
    };
    
}}//namespace