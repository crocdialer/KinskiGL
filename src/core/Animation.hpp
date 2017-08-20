// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//
//  Animation.h
//  gl
//
//  Created by Fabian on 8/17/13.
//
//

#pragma once

#include "Property.hpp"
#include "Easing.hpp"

namespace kinski{ namespace animation{
    
    DEFINE_CLASS_PTR(Animation);
    
    using ease_fn_t = std::function<float (float)>;
    using interpolate_fn_t = std::function<void (float)>;
    using callback_t = std::function<void (void)>;
    
    enum LoopType {LOOP_NONE = 0, LOOP = 1, LOOP_BACK_FORTH = 2};
    enum PlaybackType {PLAYBACK_PAUSED = 0, PLAYBACK_FORWARD = 1, PLAYBACK_BACKWARD = 2};
    
    class Animation
    {
    public:
        
        Animation();
        Animation(float duration, float delay, interpolate_fn_t interpolate_fn);
        virtual ~Animation();
        
        int getId() const;
        
        virtual float duration() const;
        
        virtual void set_duration(float d);
        
        virtual bool is_playing() const;
        virtual PlaybackType playbacktype() const;
        void set_playback_type(PlaybackType playback_type = PLAYBACK_FORWARD);
        
        LoopType loop() const;
        void set_loop(LoopType loop_type = LOOP);
        void set_interpolation_function(interpolate_fn_t fn);
        void set_ease_function(ease_fn_t fn);
        void set_start_callback(callback_t cb);
        void set_update_callback(callback_t cb);
        void set_finish_callback(callback_t cb);
        void set_reverse_start_callback(callback_t cb);
        void set_reverse_finish_callback(callback_t cb);
        
        virtual float progress() const;
        
        virtual bool finished() const;
        
        virtual void update(float timeDelta);
        
        /*!
         * Start the animation with an optional delay in seconds
         */
        virtual void start(float delay = 0.f);
        
        virtual void stop();
        
    private:
        std::shared_ptr<struct AnimationImpl> m_impl;
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
    
//    AnimationPtr create(float duration, float delay, interpolate_fn_t interpolate_fn)
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
