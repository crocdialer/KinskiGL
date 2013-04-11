#include "earth_sample.hpp"

using namespace std;
using namespace glm;

namespace kinski
{
    void Earth_App::setup()
    {
        ViewerApp::setup();
        set_precise_selection(true);
        
        /******************** add search paths ************************/
        kinski::addSearchPath("~/Desktop");
        kinski::addSearchPath("/Library/Fonts");
        
        m_font.load("Courier New Bold.ttf", 24);
        
        /*********** init our application properties ******************/
        m_map_names = Property_<std::vector<std::string> >::create("Map names",
                                                                   std::vector<std::string>());
        m_map_names->setTweakable(false);
        registerProperty(m_map_names);

        m_shader_names = Property_<std::vector<std::string> >::create("Shader names",
                                                                      std::vector<std::string>());
        m_shader_names->setTweakable(false);
        registerProperty(m_shader_names);
        
        create_tweakbar_from_component(shared_from_this());
        
        // enable observer mechanism
        observeProperties();
    
        // load state from config file
        try
        {
            Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
        }catch(Exception &e)
        {
            LOG_WARNING << e.what();
        }
        
        /********************** construct a simple scene ***********************/
        gl::Geometry::Ptr sphere(gl::createSphere(100, 36));;
        gl::MaterialPtr mat(new gl::Material);
        materials().push_back(mat);
        
        try
        {
            std::vector<string>::const_iterator it = m_map_names->value().begin();
            for (; it != m_map_names->value().end(); ++it)
            {
                m_textures.push_back(gl::createTextureFromFile(*it, true, true));
                mat->addTexture(m_textures.back());
            }
            mat->setShinyness(60);
            
            if(m_shader_names->value().size() > 1)
            {
                mat->setShader(gl::createShaderFromFile(m_shader_names->value()[0],
                                                        m_shader_names->value()[1]));
            }
            //mat->setShader(gl::createShader(gl::SHADER_UNLIT));
                           
        }catch(Exception &e)
        {
            LOG_ERROR<<e.what();
        }
        m_earth_mesh = gl::MeshPtr(new gl::Mesh(sphere, mat));
        scene().addObject(m_earth_mesh);
    }
    
    void Earth_App::update(const float timeDelta)
    {
        ViewerApp::update(timeDelta);
        
        m_earth_mesh->material()->uniform("u_time",getApplicationTime());
        m_earth_mesh->material()->uniform("u_lightDir", light_direction());
    }
    
    void Earth_App::draw()
    {
        gl::loadMatrix(gl::PROJECTION_MATRIX, camera()->getProjectionMatrix());
        gl::loadMatrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix());
        if(draw_grid()){ gl::drawGrid(500, 500, 100, 100); }
        scene().render(camera());
        
        // draw texture map(s)
        glm::vec2 offet(getWidth() - getWidth()/5.f - 10, getHeight() - 10);
        glm::vec2 step(0, - getHeight()/5.f - 10);
        for(int i = 0;i<m_textures.size();i++)
        {
            drawTexture(m_textures[i], windowSize()/5.f, offet);
            offet += step;
        }
    }
    
    // Property observer callback
    void Earth_App::updateProperty(const Property::ConstPtr &theProperty)
    {
        ViewerApp::updateProperty(theProperty);
    }
    
    void Earth_App::tearDown()
    {
        LOG_PRINT<<"ciao earth sample";
    }
}

int main(int argc, char *argv[])
{
    kinski::App::Ptr theApp(new kinski::Earth_App);
    return theApp->run();
}