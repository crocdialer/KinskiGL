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
        
        void start();
        void stop();
        bool running();
        double time_elapsed();
        double time_elapsed_for_lap();
        void reset();
        void new_lap();
        const std::vector<double>& laps();

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
        void set_callback(timer_cb_t cb);
        
    private:
        std::shared_ptr<struct timer_impl> m_impl;
    };
}