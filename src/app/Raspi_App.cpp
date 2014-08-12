#include "Raspi_App.h"
#ifdef KINSKI_RASPI

#include "core/file_functions.h"
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include "esUtil.h"

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
        esCreateWindow ( m_context.get(), getName().c_str(), getWidth(), getHeight(),
                        ES_WINDOW_RGB | ES_WINDOW_ALPHA | ES_WINDOW_DEPTH  /*| ES_WINDOW_MULTISAMPLE*/);

        // version
        LOG_INFO<<"OpenGL: " << glGetString(GL_VERSION);
        LOG_INFO<<"GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION);
        
        setWindowSize(windowSize());

        // file search paths
        kinski::addSearchPath("");
        kinski::addSearchPath("./");
        kinski::addSearchPath("./res/");
        
        // user setup hook
        setup();
    }
    
    void Raspi_App::setWindowSize(const glm::vec2 &size)
    {
        App::setWindowSize(size);
        gl::setWindowDimension(size);
        
        glViewport(0, 0, size[0], size[1]);
        if(running()) resize(size[0], size[1]);
    }
    
    void Raspi_App::swapBuffers()
    {
        eglSwapBuffers(m_context->eglDisplay, m_context->eglSurface);
    }
    
    double Raspi_App::getApplicationTime()
    {
        timeval now;
        gettimeofday(&now, NULL);
        
        return (double)(now.tv_sec - m_startTime.tv_sec + (now.tv_usec - m_startTime.tv_usec) * 1e-6);
    }
}

#endif
