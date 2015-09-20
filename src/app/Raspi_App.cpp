#include "Raspi_App.h"
#ifdef KINSKI_RASPI

#include <sys/time.h>
#include "core/file_functions.h"
//#include <GLES2/gl2.h>
//#include <EGL/egl.h>
//#include "esUtil.h"

using namespace std;

namespace kinski
{
    Raspi_App::Raspi_App(const int width, const int height):
    App(width, height),
    m_context(new ESContext)
    {
    
    }
    
    Raspi_App::~Raspi_App()
    {
        
    }
    
    // internal initialization. performed when run is invoked
    void Raspi_App::init()
    {
        gettimeofday(&m_startTime, NULL);

        esInitContext ( m_context.get() );
        esCreateWindow ( m_context.get(), get_name().c_str(), getWidth(), getHeight(),
                        ES_WINDOW_RGB | ES_WINDOW_ALPHA | ES_WINDOW_DEPTH  /*| ES_WINDOW_MULTISAMPLE*/);

        // set graphical log stream
        Logger::get()->add_outstream(&m_outstream_gl);

        // version
        LOG_INFO<<"OpenGL: " << glGetString(GL_VERSION);
        LOG_INFO<<"GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION);
        
        set_window_size(windowSize());

        // file search paths
        kinski::add_search_path("");
        kinski::add_search_path("./");
        kinski::add_search_path("./res/");
        
        // user setup hook
        setup();
    }
    
    void Raspi_App::set_window_size(const glm::vec2 &size)
    {
        App::set_window_size(size);
        gl::setWindowDimension(size);
        if(running()) resize(size[0], size[1]);
    }

    void Raspi_App::draw_internal()
    {
        draw();
        
        // draw tweakbar
        if(displayTweakBar())
        {
            // console output
            outstream_gl().draw();
        }
    }
    void Raspi_App::swapBuffers()
    {
        eglSwapBuffers(m_context->eglDisplay, m_context->eglSurface);
    }
    
    void Raspi_App::pollEvents()
    {
    
    }

    double Raspi_App::getApplicationTime()
    {
        timeval now;
        gettimeofday(&now, NULL);
        
        return (double)(now.tv_sec - m_startTime.tv_sec + (now.tv_usec - m_startTime.tv_usec) * 1e-6);
    }
}

#endif
