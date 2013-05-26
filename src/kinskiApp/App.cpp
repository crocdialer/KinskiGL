// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "App.h"
#include <boost/asio/io_service.hpp>
#include <boost/thread.hpp>

using namespace std;

namespace kinski
{
    
    App::App(const int width, const int height):
    Component("KinskiGL"),
    m_framesDrawn(0),
    m_lastMeasurementTimeStamp(0.0),
    m_framesPerSec(0.f),
    m_timingInterval(1.0),
    m_windowSize(glm::ivec2(width, height)),
    m_running(false),
    m_fullscreen(false),
    m_cursorVisible(true),
    m_io_service(new boost::asio::io_service())
    {
        srand(clock());
        m_io_work = std::shared_ptr<void>(new boost::asio::io_service::work(*m_io_service));
        m_io_thread = std::shared_ptr<boost::thread>(new boost::thread(
            boost::bind(&boost::asio::io_service::run, m_io_service.get())));
    }
    
    App::~App()
    {
        m_io_work.reset();
        m_io_service->stop();
        if(m_io_thread)
        {
            try{m_io_thread->join();}
            catch(std::exception &e){LOG_ERROR<<e.what();}
        }
    }
    
    int App::run()
    {
        try{init(); m_running = GL_TRUE;}
        catch(std::exception &e){LOG_ERROR<<e.what();}
        double timeStamp = 0.0;
        
        // Main loop
        while( m_running )
        {
            glDepthMask(GL_TRUE);
            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            // update application time
            timeStamp = getApplicationTime();
            
            // update io_service
            //m_io_service->poll();
            
            // call update callback
            update(timeStamp - m_lastTimeStamp);
            
            m_lastTimeStamp = timeStamp;
            
            // call draw callback
            draw_internal();
            
            // Swap front and back rendering buffers
            swapBuffers();
            
            // perform fps-timing
            timing(timeStamp);
            
            // Check if ESC key was pressed or window was closed or whatever
            m_running = checkRunning();
        }
        
        // manage tearDown, save stuff etc.
        tearDown();
        
        return EXIT_SUCCESS;
    }
    
    void App::draw_internal()
    {
        draw();
    };
    
    void App::timing(double timeStamp)
    {
        m_framesDrawn++;
        
        double diff = timeStamp - m_lastMeasurementTimeStamp;
        
        if(diff > m_timingInterval)
        {
            m_framesPerSec = m_framesDrawn / diff;
            m_framesDrawn = 0;
            m_lastMeasurementTimeStamp = timeStamp;
            LOG_TRACE<< m_framesPerSec << "fps -- "<<getApplicationTime()<<" sec running ...";
        }
    }
}