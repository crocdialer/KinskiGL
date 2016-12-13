// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  Timer.h
//
//  Created by Croc Dialer on 27/07/14.

#pragma once

#include "core/core.hpp"

namespace kinski
{
    class Stopwatch
    {
    public:
        Stopwatch();
        
        /*!
         * start the timer. has no effect, if the timer is already running.
         */
        void start();
        
        /*!
         * stop the timer. all measured laps are kept.
         */
        void stop();
        
        /*!
         * return true if the timer is currently running.
         */
        bool running() const;
        
        /*!
         * return the total time (in seconds) measured, including all previous laps.
         */
        double time_elapsed() const;
        
        /*!
         * return the time (in seconds) measured for the current lap.
         */
        double time_elapsed_for_lap() const;
        
        /*!
         * reset the timer. this stops time measurement, if it was running before, and clears all 
         * measured laps.
         */
        void reset();
        
        /*!
         * begin measurement of a new lap. if the timer is not running, this call has no effect.
         */
        void new_lap();
        
        /*!
         * return the values for all previously measured laps.
         */
        const std::vector<double>& laps() const;

    private:
        std::shared_ptr<struct stopwatch_impl> m_impl;
    };
    
    class Timer
    {
    public:
        typedef std::function<void(void)> timer_cb_t;
        
        Timer();
        Timer(boost::asio::io_service &io, timer_cb_t cb = timer_cb_t());
        
        /*!
         * set expiration date from now in seconds
         */
        void expires_from_now(double secs);
        
        /*!
         * get expiration date from now in seconds
         */
        double expires_from_now() const;
        
        /*!
         * returns true if the timer has expired
         */
        bool has_expired() const;
        
        /*!
         * cancel a currently running timer
         */
        void cancel();
        
        /*!
         * returns true if the timer is set to fire periodically
         */
        bool periodic() const;
        
        /*!
         * sets if the timer should fire periodically
         */
        void set_periodic(bool b = true);
        
        /*!
         * set the function object to call when the timer expires
         */
        void set_callback(timer_cb_t cb = timer_cb_t());
        
    private:
        std::shared_ptr<struct timer_impl> m_impl;
    };
}
