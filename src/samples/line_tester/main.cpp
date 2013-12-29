#include "kinskiApp/GLFW_App.h"
#include "kinskiApp/AppServer.h"

#include "kinskiGL/Mesh.h"

using namespace std;
using namespace kinski;
using namespace glm;


class LineTester : public GLFW_App
{
private:
    
    gl::Font m_font;
    Property_<float>::Ptr m_line_width = Property_<float>::create("Line width", 10.f);
    
public:
    
    /////////////////////////////////////////////////////////////////
    
    void setup()
    {
        registerProperty(m_line_width);
        
        observeProperties();
        create_tweakbar_from_component(shared_from_this());
        displayTweakBar(false);
    }
    
    /////////////////////////////////////////////////////////////////
    
    void update(float timeDelta)
    {
        //m_line_mesh->material()->uniform("u_window_size", windowSize());
    }
    
    /////////////////////////////////////////////////////////////////
    
    void draw()
    {
        gl::drawLine(windowSize() / 3.f, 2.f * windowSize() /3.f, gl::COLOR_RED, *m_line_width);
    }
    
    /////////////////////////////////////////////////////////////////
    
    void resize(int w ,int h)
    {
        
    }
    
    /////////////////////////////////////////////////////////////////
    
    void keyPress(const KeyEvent &e)
    {
        switch (e.getChar()) {
            case KeyEvent::KEY_SPACE:
                displayTweakBar(!displayTweakBar());
                break;
                
            default:
                break;
        }
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
        LOG_PRINT<<"ciao line tester";
    }
    
    /////////////////////////////////////////////////////////////////
    
    void updateProperty(const Property::ConstPtr &theProperty)
    {
        
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new LineTester);
    LOG_INFO<<"local ip: " << AppServer::local_ip();
    return theApp->run();
}
