#ifndef _KINSKI_MINI_APP_IS_INCLUDED_
#define _KINSKI_MINI_APP_IS_INCLUDED_

#include <sys/time.h>
#include "esUtil.h"
#include "App.h"
#include "OutstreamGL.h"

namespace kinski
{

class Raspi_App : public App
{
 public:
    
    Raspi_App(const int width = 1920, const int height = 1080);
    virtual ~Raspi_App();
    
    void setWindowSize(const glm::vec2 &size);
    void swapBuffers();
    double getApplicationTime();
    
    void displayTweakBar(bool b){}
    bool displayTweakBar(){return true;};
    
    const gl::OutstreamGL& outstream_gl() const {return m_outstream_gl;};
    gl::OutstreamGL& outstream_gl(){return m_outstream_gl;};

 private:

    // internal initialization. performed when run is invoked
    void init();
    void draw_internal();
    void pollEvents(){};
    
    timeval m_startTime;
    //struct ESContext;
    std::shared_ptr<ESContext> m_context;
    
    gl::OutstreamGL m_outstream_gl;
};
    
}
#endif // _KINSKI_MINI_APP_IS_INCLUDED_
