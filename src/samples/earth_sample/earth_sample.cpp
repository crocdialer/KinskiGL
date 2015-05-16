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
        kinski::add_search_path("~/Desktop");
        kinski::add_search_path("/Library/Fonts");
        
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
    
        load_settings();
        
        /********************** construct a simple scene ***********************/
        gl::Geometry::Ptr sphere = gl::Geometry::createSphere(100, 32);
        gl::MaterialPtr mat = gl::Material::create();
        materials().push_back(mat);
        
        try
        {
            std::vector<string>::const_iterator it = m_map_names->value().begin();
            for (; it != m_map_names->value().end(); ++it)
            {
                bool mip_map = true, compression = true;
                float anisoptropic_filter = 16.f;
                m_textures.push_back(gl::createTextureFromFile(*it, mip_map, compression,
                                                               anisoptropic_filter));
                mat->addTexture(m_textures.back());
            }
            mat->setShinyness(60);
            
            if(m_shader_names->value().size() > 1)
            {
                mat->setShader(gl::createShaderFromFile(m_shader_names->value()[0],
                                                        m_shader_names->value()[1]));
            }
            //mat->setShader(gl::createShader(gl::SHADER_PHONG));
                           
        }catch(Exception &e)
        {
            LOG_ERROR<<e.what();
        }
        m_earth_mesh = gl::Mesh::create(sphere, mat);
        scene().addObject(m_earth_mesh);
    }
    
    void Earth_App::update(float timeDelta)
    {
        ViewerApp::update(timeDelta);
        
        m_earth_mesh->material()->uniform("u_time",getApplicationTime());
//        m_earth_mesh->material()->uniform("u_lightDir", light_direction());
    }
    
    void Earth_App::draw()
    {
        gl::loadMatrix(gl::PROJECTION_MATRIX, camera()->getProjectionMatrix());
        gl::loadMatrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix());
        if(draw_grid()){ gl::drawGrid(500, 500, 100, 100); }
        scene().render(camera());
        
        if(selected_mesh())
        {
            gl::loadMatrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix() * selected_mesh()->transform());
            gl::drawAxes(selected_mesh());
            gl::drawBoundingBox(selected_mesh());
            if(normals()) gl::drawNormals(selected_mesh());
        }
        // draw texture map(s)
        if(displayTweakBar())
        {
            // draw maps
            float w = (windowSize()/8.f).x;
            glm::vec2 offset(getWidth() - w - 10, 10);
            for(int i = 0;i < m_textures.size();i++)
            {
                float h = m_textures[i].getHeight() * w / m_textures[i].getWidth();
                glm::vec2 step(0, h + 10);
                drawTexture(m_textures[i], vec2(w, h), offset);
                gl::drawText2D(as_string(m_textures[i].getWidth()) + std::string(" x ") +
                               as_string(m_textures[i].getHeight()), m_font, glm::vec4(1),
                               offset);
                offset += step;
            }
            // draw fps string
            gl::drawText2D(kinski::as_string(framesPerSec()), m_font,
                           vec4(vec3(1) - clear_color().xyz(), 1.f),
                           glm::vec2(windowSize().x - 110, windowSize().y - 40));
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