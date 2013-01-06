#include "App.h"

using namespace std;

namespace kinski
{
    
    App::App(const int width, const int height):
    Component("KinskiGL"),
    m_framesDrawn(0),
    m_lastMeasurementTimeStamp(0.0),
    m_timingInterval(2.0),
    m_windowSize(glm::ivec2(width, height)),
    m_running(false),
    m_fullscreen(false),
    m_cursorVisible(true)
    {
        
    }
    
    App::~App()
    {

    }
    
    int App::run()
    {
        init();
        
        m_running = GL_TRUE;
        
        double timeStamp = 0.0;
        
        // Main loop
        while( m_running )
        {
            glDepthMask(GL_TRUE);
            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            // update application time
            timeStamp = getApplicationTime();
            
            // call update callback
            update(timeStamp - m_lastTimeStamp);
            
            m_lastTimeStamp = timeStamp;
            
            // call draw callback
            draw_internal();
            
            // Swap front and back rendering buffers
            swapBuffers();
            
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
        
        m_lastMeasurementTimeStamp += timeStamp;
        
        if(m_lastMeasurementTimeStamp > m_timingInterval)
        {
            float fps = m_framesDrawn / m_lastMeasurementTimeStamp;
            m_framesDrawn = 0;
            m_lastMeasurementTimeStamp = 0;
        }
    }
}