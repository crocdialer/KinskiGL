// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "App.hpp"

#include <thread>
#include <mutex>
#include <boost/asio/io_service.hpp>

using namespace std;

namespace kinski
{
    // threadsafe task-counting internals
    namespace
    {
        uint32_t num_tasks = 0;
        std::mutex mutex;
    }
    
    App::App(int argc, char *argv[]):
    Component(argc ? kinski::get_filename_part(argv[0]) : "KinskiApp"),
    m_framesDrawn(0),
    m_lastMeasurementTimeStamp(0.0),
    m_framesPerSec(0.f),
    m_timingInterval(1.0),
    m_running(false),
    m_fullscreen(false),
    m_displayTweakBar(false),
    m_cursorVisible(true),
    m_max_fps(60.f),
    m_main_queue(0),
    m_background_queue(2)
    {
        srand(clock());
        for(int i = 0; i < argc; i++){ m_args.push_back(argv[i]); }
    }
    
    App::~App()
    {
        
    }
    
    int App::run()
    {
        try{init(); m_running = GL_TRUE;}
        catch(std::exception &e){LOG_ERROR<<e.what();}
        double timeStamp = 0.0;
        
        // Main loop
        while( m_running )
        {            
            // update application time
            timeStamp = getApplicationTime();
            
            // poll io_service if no seperate worker-threads exist
            if(!m_main_queue.get_num_threads()) m_main_queue.io_service().poll();
            
            // poll input events
            pollEvents();
            
            // time elapsed since last frame
            float time_delta = timeStamp - m_lastTimeStamp;
            
            // call update callback
            update(time_delta);
            
            m_lastTimeStamp = timeStamp;
            
            // call draw callback
            draw_internal();
            
            // Swap front and back rendering buffers
            swapBuffers();
            
            // perform fps-timing
            timing(timeStamp);
            
            // Check if ESC key was pressed or window was closed or whatever
            m_running = checkRunning();
            
            // fps managment
            float current_fps = 1.f / time_delta;
            
            if(current_fps > m_max_fps)
            {
                double sleep_secs = std::max(0.0, (1.0 / m_max_fps - time_delta));
                this_thread::sleep_for(std::chrono::nanoseconds((long)(sleep_secs * 1000000000L)));
            }
        }
        
        // manage tearDown, save stuff etc.
        tearDown();
        
        return EXIT_SUCCESS;
    }
    
    void App::set_window_size(const glm::vec2 &size)
    {
        if(running())
            gl::set_window_dimension(size);
    }
    
    void App::draw_internal()
    {
        draw();
    }
    
    void App::timing(double timeStamp)
    {
        m_framesDrawn++;
        
        double diff = timeStamp - m_lastMeasurementTimeStamp;
        
        if(diff > m_timingInterval)
        {
            m_framesPerSec = m_framesDrawn / diff;
            m_framesDrawn = 0;
            m_lastMeasurementTimeStamp = timeStamp;
        }
    }
    
    void App::inc_task()
    {
        std::unique_lock<std::mutex> lock(mutex);
        num_tasks++;
    }
    
    void App::dec_task()
    {
        if(num_tasks)
        {        
            std::unique_lock<std::mutex> lock(mutex);
            num_tasks--;
        }
    }
    
    bool App::is_loading() const
    {
        return num_tasks;
    }
}