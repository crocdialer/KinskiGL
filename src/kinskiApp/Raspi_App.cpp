#ifdef KINSKI_RASPI

#include "kinskiCore/file_functions.h"
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include "esUtil.h"
#include "MiniApp.h"

using namespace std;

namespace kinski
{
    RasPi_App::RasPi_App(const int width, const int height):
    App(width, height),
    m_context(new ESContext)
    {
    
    }
    
    RasPi_App::~RasPi_App()
    {
        
    }
    
    // internal initialization. performed when run is invoked
    void RasPi_App::init()
    {
        gettimeofday(&m_startTime, NULL);
        
        esInitContext ( m_context.get() );
        esCreateWindow ( m_context.get(), getName(), getWidth(), getHeight(),
                        ES_WINDOW_RGB | ES_WINDOW_ALPHA | ES_WINDOW_DEPTH /* | ES_WINDOW_MULTISAMPLE*/);
        
        // version
        LOG_INFO<<"OpenGL: " << glGetString(GL_VERSION);
        LOG_INFO<<"GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION);
        
        // file search paths
        kinski::addSearchPath("");
        kinski::addSearchPath("./");
        kinski::addSearchPath("./res/");
        
    }
    
    void RasPi_App::setWindowSize(const glm::ivec2 size)
    {
    
    }
    
    void RasPi_App::swapBuffers()
    {
        eglSwapBuffers(m_context->eglDisplay, m_context->eglSurface);
    }
    
    double RasPi_App::getApplicationTime()
    {
        timeval now;
        gettimeofday(&now, NULL);
        
        return (double)(now.tv_sec - m_startTime.tv_sec + (now.tv_usec - m_startTime.tv_usec) * 1e-6);
    }
}

#endif