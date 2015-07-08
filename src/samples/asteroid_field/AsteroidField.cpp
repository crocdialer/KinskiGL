//
//  AsteroidField.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "AsteroidField.h"

#include "gl/Visitor.h"

// module headers
#include "assimp/AssimpConnector.h"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void AsteroidField::setup()
{
    ViewerApp::setup();
    registerProperty(m_model_folder);
    registerProperty(m_sky_box_path);
    registerProperty(m_half_extents);
    registerProperty(m_velocity);
    observeProperties();
    create_tweakbar_from_component(shared_from_this());
    
    m_light_component = std::make_shared<LightComponent>();
    m_light_component->set_lights(lights());
    create_tweakbar_from_component(m_light_component);
    
    // add lights to scene
    for (auto l : lights()){ scene().addObject(l ); }
    
    m_skybox_mesh = gl::Mesh::create(gl::Geometry::createSphere(1.f, 24), gl::Material::create());
    m_skybox_mesh->material()->setDepthWrite(false);
    m_skybox_mesh->material()->setTwoSided();
    
//    m_spawn_timer = Timer(io_service(), [](){ });
//    m_spawn_timer.expires_from_now(3.f);
    
    // finally load state from file
    load_settings();
    
    for(int i = 0; i < 200; i++)
    {    
        auto test_mesh = m_proto_objects[0]->copy();
        
        // random spawn position
        test_mesh->setPosition(glm::linearRand(m_aabb.min, m_aabb.max));
        
        // object rotation via update-functor
        vec3 rot_vec = glm::ballRand(1.f);
        float rot_speed = random<float>(10.f, 90.f);
        test_mesh->set_update_function([test_mesh, rot_vec, rot_speed](float t)
        {
            test_mesh->transform() = glm::rotate(test_mesh->transform(), rot_speed * t, rot_vec);
        });
        scene().addObject(test_mesh);
    }
}

/////////////////////////////////////////////////////////////////

void AsteroidField::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    // fetch all model-objects in scene
    gl::SelectVisitor<gl::Mesh> mv;
    scene().root()->accept(mv);
    
    for(auto &m : mv.getObjects())
    {
        // translation update
        m->position() += m_velocity->value() * timeDelta;
        
        // reposition within AABB if necessary
        if(!m_aabb.contains(m->position()))
        {
            auto &p = m->position();
            
            // find out-of-bounds dimension
            if(p.x < m_aabb.min.x){ p.x += m_aabb.width(); }
            else if(p.x > m_aabb.max.x){ p.x -= m_aabb.width(); }
            if(p.y < m_aabb.min.y){ p.y += m_aabb.height(); }
            else if(p.y > m_aabb.max.y){ p.y -= m_aabb.height(); }
            if(p.z < m_aabb.min.z){ p.z += m_aabb.depth(); }
            else if(p.z > m_aabb.max.z){ p.z -= m_aabb.depth(); }
        }
    }
}

/////////////////////////////////////////////////////////////////

void AsteroidField::draw()
{
    // skybox drawing
    gl::setProjection(camera());
    mat4 m = camera()->getViewMatrix();
    m[3] = vec4(0, 0, 0, 1);
    gl::loadMatrix(gl::MODEL_VIEW_MATRIX, m);
    gl::drawMesh(m_skybox_mesh);
    
    /////////////////////////////////////////////////////////
    
    // draw asteroid field
    gl::setMatrices(camera());
    if(*m_draw_grid){ gl::drawGrid(50, 50); }
    
    scene().render(camera());
}

/////////////////////////////////////////////////////////////////

void AsteroidField::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::got_message(const std::vector<uint8_t> &the_message)
{
    LOG_INFO<<string(the_message.begin(), the_message.end());
}

/////////////////////////////////////////////////////////////////

void AsteroidField::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void AsteroidField::tearDown()
{
    LOG_PRINT<<"ciao empty sample";
}

/////////////////////////////////////////////////////////////////

void AsteroidField::updateProperty(const Property::ConstPtr &theProperty)
{
    ViewerApp::updateProperty(theProperty);
    
    if(theProperty == m_model_folder)
    {
        m_proto_objects.clear();
        
        add_search_path(get_directory_part(*m_model_folder));
        for (const auto &p : get_directory_entries(*m_model_folder))
        {
            if(get_filetype(p) != FileType::FILE_MODEL){ continue; }
            auto mesh = gl::AssimpConnector::loadModel(p);
            if(mesh)
            {
                auto &verts = mesh->geometry()->vertices();
                vec3 centroid = gl::calculateCentroid(verts);
                for(auto &v : verts){ v -= centroid; }
                
                mesh->material()->setShader(gl::createShader(gl::SHADER_GOURAUD));
                m_proto_objects.push_back(mesh);
            }
        }
    }
    else if(theProperty == m_sky_box_path)
    {
        auto &tex_vec = m_skybox_mesh->material()->textures();
        tex_vec.clear();
        try
        {
            tex_vec.push_back(gl::createTextureFromFile(*m_sky_box_path));
        }
        catch (Exception &e){ LOG_WARNING << e.what(); }
    }
    else if(theProperty == m_half_extents)
    {
        m_aabb = gl::AABB(-m_half_extents->value(), m_half_extents->value());
    }
}
