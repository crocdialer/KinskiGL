//
//  ModelViewer.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "ModelViewer.h"
#include "AssimpConnector.h"

using namespace std;
using namespace kinski;
using namespace glm;

/////////////////////////////////////////////////////////////////

void ModelViewer::setup()
{
    ViewerApp::setup();
    
    registerProperty(m_model_path);
    observeProperties();
    create_tweakbar_from_component(shared_from_this());
    
    m_light_component = std::make_shared<LightComponent>();
    m_light_component->set_lights(lights());
    create_tweakbar_from_component(m_light_component);
    
    // add lights to scene
    for (auto l : lights()){ scene().addObject(l ); }
    
    // add groundplane
    auto ground_mesh = gl::Mesh::create(gl::Geometry::createPlane(400, 400),
                                        gl::Material::create(gl::createShader(gl::SHADER_PHONG)));
    ground_mesh->transform() = glm::rotate(mat4(), -90.f, gl::X_AXIS);
    uint8_t c = 255;
    gl::Texture t(&c, GL_RED, 1, 1);
    ground_mesh->material()->addTexture(t);
    
    scene().addObject(ground_mesh);
    
    load_settings();
    m_light_component->refresh();
}

/////////////////////////////////////////////////////////////////

void ModelViewer::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::draw()
{
    gl::setMatrices(camera());
    if(draw_grid()){ gl::drawGrid(50, 50); }
    
    if(m_light_component->draw_light_dummies())
    {
        for (auto l : lights()){ gl::drawLight(l); }
    }
    
    scene().render(camera());
    
    // draw texture map(s)
    if(displayTweakBar() && m_mesh)
    {
        gl::MeshPtr m = m_mesh;
        if(selected_mesh()){ m = selected_mesh(); }
        draw_textures(m->material()->textures());
    }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::got_message(const std::vector<uint8_t> &the_message)
{
    LOG_INFO<<string(the_message.begin(), the_message.end());
}

/////////////////////////////////////////////////////////////////

void ModelViewer::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    std::vector<gl::Texture> dropped_textures;
    
    for(const string &f : files)
    {
        LOG_INFO << f;
        
        
        // add path to searchpaths
        kinski::add_search_path(kinski::get_directory_part(f));
        
        switch (get_filetype(f))
        {
            case FileType::FILE_MODEL:
                *m_model_path = f;
                break;
            
            case FileType::FILE_IMAGE:
                try
                {
                    dropped_textures.push_back(gl::createTextureFromFile(f, true, false));
                }
                catch (Exception &e) { LOG_WARNING << e.what();}
                if(scene().pick(gl::calculateRay(camera(), vec2(e.getX(), e.getY()))))
                {
                    LOG_INFO << "texture drop on model";
                }
                break;
            default:
                break;
        }
    }
    if(m_mesh){ m_mesh->material()->textures() = dropped_textures; }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::tearDown()
{
    LOG_PRINT<<"ciao model viewer";
}

/////////////////////////////////////////////////////////////////

void ModelViewer::updateProperty(const Property::ConstPtr &theProperty)
{
    ViewerApp::updateProperty(theProperty);
    
    if(theProperty == m_model_path)
    {
        add_search_path(get_directory_part(*m_model_path));
        gl::MeshPtr m = gl::AssimpConnector::loadModel(*m_model_path);
        
        if(m)
        {
            scene().removeObject(m_mesh);
//            m = gl::Mesh::create(gl::Geometry::createBox(vec3(2.f)), gl::Material::create());
            m_mesh = m;
            
            scene().addObject(m_mesh);

            
//            m->material()->setShader(gl::createShader(gl::SHADER_PHONG_NORMALMAP));
//            m->material()->addTexture(gl::createTextureFromFile("~/Desktop/normal.png", true));
            
            auto aabb = m->boundingBox();
            float scale_factor = 50.f / aabb.width();
            m->setScale(scale_factor);
        }
    }
}
