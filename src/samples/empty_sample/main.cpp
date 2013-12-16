#include "kinskiApp/GLFW_App.h"


using namespace std;
using namespace kinski;
using namespace glm;


class Empty_App : public GLFW_App
{
private:
    
    gl::Font m_font;
    
public:
    
    /////////////////////////////////////////////////////////////////
    
    void setup()
    {
        
    }
    
    /////////////////////////////////////////////////////////////////
    
    void update(float timeDelta)
    {
       
    }
    
    /////////////////////////////////////////////////////////////////
    
    void draw()
    {
        
    }
    
    /////////////////////////////////////////////////////////////////
    
    void resize(int w ,int h)
    {
        
    }
    
    /////////////////////////////////////////////////////////////////
    
    void keyPress(const KeyEvent &e)
    {
        
    }
    
    /////////////////////////////////////////////////////////////////
    
    void keyRelease(const KeyEvent &e)
    {
        
    }
    
    /////////////////////////////////////////////////////////////////
    
    void mousePress(const MouseEvent &e)
    {
    
    }
    
    /////////////////////////////////////////////////////////////////
    
    void mouseRelease(const MouseEvent &e)
    {
    
    }
    
    /////////////////////////////////////////////////////////////////
    
    void mouseMove(const MouseEvent &e)
    {
    
    }
    
    /////////////////////////////////////////////////////////////////
    
    void mouseDrag(const MouseEvent &e)
    {
    
    }
    
    /////////////////////////////////////////////////////////////////
    
    void mouseWheel(const MouseEvent &e)
    {
    
    }
    
    /////////////////////////////////////////////////////////////////
    
    void got_message(const std::string &the_message)
    {
        LOG_INFO<<the_message;
    }
    
    /////////////////////////////////////////////////////////////////
    
    void tearDown()
    {
        LOG_PRINT<<"ciao empty app";
    }
    
    /////////////////////////////////////////////////////////////////
    
    void updateProperty(const Property::ConstPtr &theProperty)
    {
        
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new Empty_App);
    LOG_INFO<<"local ip: " << AppServer::local_ip();
    return theApp->run();
}
