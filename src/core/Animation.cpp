//
//  Animation.h
//  gl
//
//  Created by Fabian on 8/17/13.
//
//

#include "Animation.hpp"

using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::steady_clock;

// ratio is 1 second per second, wow :D
typedef std::chrono::duration<float> float_second;

namespace kinski{ namespace animation{

    struct Animation::AnimationImpl
    {
        int id;
        PlaybackType playback_type;
        LoopType loop_type;
        steady_clock::time_point start_time, end_time, current_time;
        EaseFunction ease_fn;
        InterpolationFunction interpolate_fn;
        Callback start_fn, update_fn, finish_fn, reverse_start_fn, reverse_finish_fn;
        
        AnimationImpl():
        playback_type(PLAYBACK_PAUSED),
        loop_type(LOOP_NONE),
        start_time(steady_clock::now()),
        end_time(start_time),
        current_time(start_time),
        ease_fn(EaseNone()),
        interpolate_fn([](float){}){}
        
        AnimationImpl(float duration, float delay, InterpolationFunction interpolate_fn):
        playback_type(PLAYBACK_PAUSED),
        loop_type(LOOP_NONE),
        start_time(steady_clock::now()),
        end_time(start_time + duration_cast<steady_clock::duration>(float_second(duration))),
        current_time(start_time),
        ease_fn(EaseNone()),
        interpolate_fn(interpolate_fn){}
    };
    
    Animation::Animation():m_impl(new AnimationImpl)
    {}
    
    Animation::Animation(float duration, float delay, InterpolationFunction interpolate_fn):
    m_impl(new AnimationImpl(duration, delay, interpolate_fn))
    {}
    
    Animation::~Animation()
    {
        switch (playbacktype())
        {
            case PLAYBACK_FORWARD:
                if(m_impl->finish_fn)
                    m_impl->finish_fn();
                break;
                
            case PLAYBACK_BACKWARD:
                if(m_impl->reverse_finish_fn)
                    m_impl->reverse_finish_fn();
                break;
                
            case PLAYBACK_PAUSED:
            default:
                break;
        }
    }
    
    int Animation::getId() const {return m_impl->id;}
    
    float Animation::duration() const
    {return duration_cast<float_second>(m_impl->end_time - m_impl->start_time).count();}
    
    void Animation::set_duration(float d)
    {
        m_impl->end_time = m_impl->start_time + duration_cast<steady_clock::duration>(float_second(d));
    };
    
    bool Animation::is_playing() const
    {
        return m_impl->playback_type && (m_impl->current_time >= m_impl->start_time);
    }
    
    PlaybackType Animation::playbacktype() const
    {
        return m_impl->playback_type;
    }
    
    void Animation::set_playback_type(PlaybackType playback_type){m_impl->playback_type = playback_type;}
    
    LoopType Animation::loop() const {return m_impl->loop_type;}
    
    void Animation::set_loop(LoopType loop_type)
    {m_impl->loop_type = loop_type;}
    
//    steady_clock::time_point Animation::start_time() const {return m_impl->start_time;}
//    steady_clock::time_point Animation::end_time() const {return m_impl->end_time;}
    
    void Animation::set_interpolation_function(InterpolationFunction fn)
    {
        m_impl->interpolate_fn = fn;
    }
    
    void Animation::set_ease_function(EaseFunction fn){m_impl->ease_fn = fn;}
    void Animation::set_start_callback(Callback cb){m_impl->start_fn = cb;}
    void Animation::set_update_callback(Callback cb){m_impl->update_fn = cb;}
    void Animation::set_finish_callback(Callback cb){m_impl->finish_fn = cb;}
    void Animation::set_reverse_start_callback(Callback cb){m_impl->reverse_start_fn = cb;}
    void Animation::set_reverse_finish_callback(Callback cb){m_impl->reverse_finish_fn = cb;}
    
    float Animation::progress() const
    {
        return clamp(duration_cast<float_second>(m_impl->current_time - m_impl->start_time).count() /
                     duration_cast<float_second>(m_impl->end_time - m_impl->start_time).count(),
                     0.f, 1.f);
    }
    
    bool Animation::finished() const
    {
        return m_impl->current_time >= m_impl->end_time;
    }
    
    void Animation::update(float timeDelta)
    {
        // update timing
        m_impl->current_time += duration_cast<steady_clock::duration>(float_second(timeDelta));
        
        if(!is_playing()) return;
        
        if(finished())
        {
            // fire finish callback, if any
            if(m_impl->playback_type == PLAYBACK_FORWARD && m_impl->finish_fn)
                m_impl->finish_fn();
            else if(m_impl->playback_type == PLAYBACK_BACKWARD && m_impl->reverse_finish_fn)
                m_impl->reverse_finish_fn();
            
            if(loop())
            {
                if(m_impl->loop_type == LOOP_BACK_FORTH)
                {
                    m_impl->playback_type = m_impl->playback_type == PLAYBACK_FORWARD ?
                        PLAYBACK_BACKWARD : PLAYBACK_FORWARD;
                }
                start();
            }
            else
            {
                // end playback
                stop();
            }
        }
        
        // this applies easing and passes it to an interpolation function
        float val = m_impl->ease_fn(progress());
        
        if(m_impl->playback_type == PLAYBACK_BACKWARD){ val = 1.f - val; }
        m_impl->interpolate_fn(val);
            
        // fire update callback, if any
        if(m_impl->update_fn)
            m_impl->update_fn();
    };
    
    /*!
     * Start the animation with an optional delay in seconds
     */
    void Animation::start(float delay)
    {
        if(!is_playing())
            m_impl->playback_type = PLAYBACK_FORWARD;
        
        float dur = duration();
        m_impl->current_time = steady_clock::now();
        m_impl->start_time = m_impl->current_time + duration_cast<steady_clock::duration>(float_second(delay));
        m_impl->end_time = m_impl->start_time + duration_cast<steady_clock::duration>(float_second(dur));
        
        // fire start callback, if any
        if(m_impl->playback_type == PLAYBACK_FORWARD && m_impl->start_fn)
            m_impl->start_fn();
        else if(m_impl->playback_type == PLAYBACK_BACKWARD && m_impl->reverse_start_fn)
            m_impl->reverse_start_fn();
    };
    
    void Animation::stop()
    {
        m_impl->playback_type = PLAYBACK_PAUSED;
    }
    
    //////////////////////////////////////////////////////////////////////////////////////////
    
    CompoundAnimation::CompoundAnimation():Animation(){}
    
    void CompoundAnimation::start(float delay)
    {
        for(auto &child_anim : m_animations)
            child_anim.start();
    }
    
    void CompoundAnimation::stop()
    {
        for(auto &child_anim : m_animations)
            child_anim.stop();
    }
    
    float CompoundAnimation::duration() const
    {
        if(m_animations.empty()) return 0.f;
        
        // find min start time and max end time
        steady_clock::time_point start_tp = steady_clock::time_point::max(),
        end_tp = steady_clock::time_point::min();
        
//        for(auto &child_anim : m_animations)
//        {
//            if(child_anim.start_time() < start_tp)
//                start_tp = child_anim.start_time();
//            if(child_anim.end_time() > end_tp)
//                end_tp = child_anim.end_time();
//        }
        return duration_cast<float_second>(end_tp - start_tp).count();
    }
    
    void CompoundAnimation::update(float timeDelta)
    {
        for(auto &child_anim : m_animations){ child_anim.update(timeDelta); }
    }
    
    bool CompoundAnimation::finished() const
    {
        for(auto &child_anim : m_animations)
        {
            if(!child_anim.finished())
                return false;
        }
        return true;
    }
    
}}//namespaces