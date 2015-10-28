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
    register_property(m_model_folder);
    register_property(m_texture_folder);
    register_property(m_num_objects);
    register_property(m_sky_box_path);
    register_property(m_half_extents);
    register_property(m_velocity);
    register_property(m_mode);
    observe_properties();
    
    create_tweakbar_from_component(shared_from_this());
    create_tweakbar_from_component(m_light_component);
    
    m_skybox_mesh = gl::Mesh::create(gl::Geometry::createSphere(1.f, 16), gl::Material::create());
    m_skybox_mesh->material()->setDepthWrite(false);
    m_skybox_mesh->material()->setTwoSided();
    
    // finally load state from file
    load_settings();
    
    // and load our assets
    load_assets();
}

/////////////////////////////////////////////////////////////////

void AsteroidField::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    if(m_dirty_flag)
    {
        m_dirty_flag = false;
        create_scene(*m_num_objects);
    }
    
    switch (*m_mode)
    {
        case MODE_NORMAL:
            lights()[0]->set_enabled(true);
            break;
            
        case MODE_LIGHTSPEED:
            lights()[0]->set_enabled(false);
            break;
            
        default:
            break;
    }
    
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
    if(*m_mode == MODE_NORMAL)
    {
        gl::setProjection(camera());
        mat4 m = camera()->getViewMatrix();
        m[3] = vec4(0, 0, 0, 1);
        gl::loadMatrix(gl::MODEL_VIEW_MATRIX, m);
        gl::drawMesh(m_skybox_mesh);
    }
    
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
    LOG_PRINT<<"ciao asteroid field";
}

/////////////////////////////////////////////////////////////////

void AsteroidField::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
    
    if(theProperty == m_model_folder)
    {
        
    }
    else if(theProperty == m_texture_folder)
    {
        
    }
    else if(theProperty == m_sky_box_path)
    {
        auto &tex_vec = m_skybox_mesh->material()->textures();
        tex_vec.clear();
        try
        {
            tex_vec.push_back(gl::createTextureFromFile(*m_sky_box_path, true, true));
        }
        catch (Exception &e){ LOG_WARNING << e.what(); }
    }
    else if(theProperty == m_half_extents)
    {
        m_dirty_flag = true;
        m_aabb = gl::AABB(-m_half_extents->value(), m_half_extents->value());
    }
    else if(theProperty == m_num_objects)
    {
        m_dirty_flag = true;
    }
}

void AsteroidField::load_assets()
{
    m_proto_objects.clear();
    
    add_search_path(*m_model_folder);
    add_search_path(*m_texture_folder);
    auto shader = gl::createShader(gl::ShaderType::GOURAUD);
    
    for (const auto &p : get_directory_entries(*m_model_folder, FileType::MODEL))
    {
        auto mesh = gl::AssimpConnector::loadModel(p);
        if(mesh)
        {
            auto &verts = mesh->geometry()->vertices();
            vec3 centroid = gl::calculateCentroid(verts);
            for(auto &v : verts){ v -= centroid; }
            mesh->geometry()->createGLBuffers();
            mesh->geometry()->computeBoundingBox();
            
            mesh->material()->setShader(shader);
            mesh->material()->setAmbient(gl::COLOR_WHITE);
            
            auto aabb = mesh->boundingBox();
            float scale_factor = 50.f / aabb.width();
            mesh->setScale(scale_factor);
            
            m_proto_objects.push_back(mesh);
        }
    }
    
    m_proto_textures.clear();
    
    for(auto &p : get_directory_entries(*m_texture_folder))
    {
        try
        {
            m_proto_textures.push_back(gl::createTextureFromFile(p, true, true));
        }
        catch (Exception &e){ LOG_WARNING << e.what(); }
    }
}

void AsteroidField::create_scene(int num_objects)
{
    scene().clear();
    
    // add lights to scene
    for (auto l : lights()){ scene().addObject(l ); }
    m_light_component->set_lights(lights());
    
    int m = 0;
    
    for(int i = 0; i < num_objects; i++)
    {
        auto test_mesh = m_proto_objects[m % m_proto_objects.size()]->copy();
        test_mesh->setScale(test_mesh->scale() * random<float>(.5f, 3.f));
        m++;
        
        if(test_mesh->material()->textures().empty())
        {
            test_mesh->material()->addTexture(m_proto_textures[m % m_proto_textures.size()]);
        }
        
        // random spawn position
        test_mesh->setPosition(glm::linearRand(m_aabb.min, m_aabb.max));
        
        // object rotation via update-functor
        vec3 rot_vec = glm::ballRand(1.f);
        float rot_speed = glm::radians(random<float>(10.f, 90.f));
        test_mesh->set_update_function([test_mesh, rot_vec, rot_speed](float t)
        {
            test_mesh->transform() = glm::rotate(test_mesh->transform(), rot_speed * t, rot_vec);
        });
        scene().addObject(test_mesh);
    }
}
