#include "app/GLFW_App.h"
#include "gl/Mesh.h"

#include "core/networking.h"

using namespace std;
using namespace kinski;
using namespace glm;


class LineTester : public GLFW_App
{
private:
    
    gl::Font m_font;
    Property_<float>::Ptr m_line_width = Property_<float>::create("Line width", 10.f);
    Property_<gl::Color>::Ptr m_line_color = Property_<gl::Color>::create("Line color",
                                                                          gl::COLOR_WHITE);
    Property_<bool>::Ptr m_use_texture = Property_<bool>::create("Use texture", false);
    Property_<string>::Ptr m_texture_name = Property_<string>::create("Texture name", "pattern2.png");
    
    gl::MaterialPtr m_line_material;
    
    vector<vec3> m_drag_buffer;
    
public:
    
    /////////////////////////////////////////////////////////////////
    
    void setup()
    {
        m_line_material = gl::Material::create(gl::createShader(gl::SHADER_LINES_2D));
        
        registerProperty(m_line_width);
        registerProperty(m_line_color);
        registerProperty(m_use_texture);
        registerProperty(m_texture_name);
        
        observeProperties();
        create_tweakbar_from_component(shared_from_this());
        displayTweakBar(false);
        
        m_texture_name->notifyObservers();
        
        
    }
    
    /////////////////////////////////////////////////////////////////
    
    void update(float timeDelta)
    {
        
    }
    
    /////////////////////////////////////////////////////////////////
    
    void draw()
    {
        //gl::drawLine(windowSize() / 5.f, 4.f * windowSize() / 5.f, *m_line_color, *m_line_width);
        
        gl::setMatricesForWindow();
        gl::drawLines({vec3(windowSize() / 5.f, 0), vec3(4.f * windowSize() / 5.f, 0)},
                      m_line_material, *m_line_width);
    }
    
    /////////////////////////////////////////////////////////////////
    
    void resize(int w ,int h)
    {
        
    }
    
    /////////////////////////////////////////////////////////////////
    
    void keyPress(const KeyEvent &e)
    {
        switch (e.getChar())
        {
            case GLFW_KEY_SPACE:
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

    
    /////////////////////////////////////////////////////////////////
    
    void tearDown()
    {
        LOG_PRINT<<"ciao line tester";
    }
    
    /////////////////////////////////////////////////////////////////
    
    void updateProperty(const Property::ConstPtr &theProperty)
    {
        if(theProperty == m_texture_name)
        {
            m_line_material->textures().clear();
            
            try{m_line_material->addTexture(gl::createTextureFromFile(*m_texture_name));}
            catch(Exception &e){LOG_ERROR<<e.what();}
        }
        else if(theProperty == m_line_color)
        {
            m_line_material->setDiffuse(*m_line_color);
            m_line_material->setBlending(true);
        }
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new LineTester);
    LOG_INFO<<"local ip: " << net::local_ip();
    return theApp->run();
}
