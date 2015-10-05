#ifndef _KINSKI_MINI_APP_IS_INCLUDED_
#define _KINSKI_MINI_APP_IS_INCLUDED_

#include "esUtil.h"

#undef countof
#include "App.h"
#include "OutstreamGL.h"

namespace kinski
{

class Raspi_App : public App
{
 public:
    
    Raspi_App(const int width = 1920, const int height = 1080);
    virtual ~Raspi_App();
    
    void set_window_size(const glm::vec2 &size) override;
    double getApplicationTime() override;
    
    const gl::OutstreamGL& outstream_gl() const {return m_outstream_gl;};
    gl::OutstreamGL& outstream_gl(){return m_outstream_gl;};

 private:

    // internal initialization. performed when run is invoked
    void init() override;
    void draw_internal() override;
    void swapBuffers() override;
    void pollEvents() override;
    
    timeval m_startTime;
    //struct ESContext;
    std::shared_ptr<ESContext> m_context;
    
    gl::OutstreamGL m_outstream_gl;
};
    
}
#endif // _KINSKI_MINI_APP_IS_INCLUDED_
