#ifndef _KINSKI_MINI_APP_IS_INCLUDED_
#define _KINSKI_MINI_APP_IS_INCLUDED_

#include <sys/time.h>
#include "esUtil.h"
#include "App.h"

namespace kinski
{

class Raspi_App : public App
{
 public:
    
    Raspi_App(const int width = 1920, const int height = 1080);
    virtual ~Raspi_App();
    
    void setWindowSize(const glm::ivec2 size);
    void swapBuffers();
    double getApplicationTime();
    
 private:

    // internal initialization. performed when run is invoked
    void init();
    
    timeval m_startTime;
    //struct ESContext;
    std::shared_ptr<ESContext> m_context;
};
    
}
#endif // _KINSKI_MINI_APP_IS_INCLUDED_
