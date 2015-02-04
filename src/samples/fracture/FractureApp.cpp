//
//  FractureApp.cpp
//  gl
//
//  Created by Fabian on 01/02/15.
//
//

#include "FractureApp.h"
#include "AssimpConnector.h"

using namespace std;
using namespace kinski;
using namespace glm;

/////////////////////////////////////////////////////////////////

void FractureApp::setup()
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
    
    // init physics
    m_physics.init();
    m_physics.set_world_boundaries(vec3(500, 500, 500), vec3(0, -250, 0));
    
    load_settings();
    m_light_component->refresh();
}

/////////////////////////////////////////////////////////////////

void FractureApp::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    m_physics.step_simulation(timeDelta);
}

/////////////////////////////////////////////////////////////////

void FractureApp::draw()
{
    gl::setMatrices(camera());
    if(draw_grid()){ gl::drawGrid(50, 50); }
    
    if(m_light_component->draw_light_dummies())
    {
        for (auto l : lights()){ gl::drawLight(l); }
    }
    
    scene().render(camera());
    
    m_physics.debug_render(camera());
    
    // draw texture map(s)
    if(displayTweakBar()){ draw_textures(); }
}

/////////////////////////////////////////////////////////////////

void FractureApp::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void FractureApp::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
}

/////////////////////////////////////////////////////////////////

void FractureApp::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void FractureApp::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
    
    if(e.isRight())
    {
        auto ray = gl::calculateRay(camera(), e.getX(), e.getY());
        shoot_box();
    }
}

/////////////////////////////////////////////////////////////////

void FractureApp::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void FractureApp::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void FractureApp::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void FractureApp::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void FractureApp::got_message(const std::vector<uint8_t> &the_message)
{
    LOG_INFO<<string(the_message.begin(), the_message.end());
}

/////////////////////////////////////////////////////////////////

void FractureApp::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    textures().clear();
    
    for(const string &f : files)
    {
        LOG_INFO << f;

        // add path to searchpaths
        kinski::addSearchPath(kinski::getDirectoryPart(f));
        m_search_paths->value().push_back(f);
        
        switch (get_filetype(f))
        {
            case FileType::FILE_MODEL:
                *m_model_path = f;
                break;
            
            case FileType::FILE_IMAGE:
                try
                {
                    textures().push_back(gl::createTextureFromFile(f, true, false));
                    
                    if(m_mesh)
                    {
                        m_mesh->material()->textures().clear();
                        m_mesh->material()->textures().push_back(textures().back());
                    }
                }
                catch (Exception &e) { LOG_WARNING << e.what(); }
                if(scene().pick(gl::calculateRay(camera(), e.getX(), e.getY())))
                {
                    LOG_INFO << "texture drop on model";
                }
                break;
            default:
                break;
        }
    }
}

/////////////////////////////////////////////////////////////////

void FractureApp::tearDown()
{
    LOG_PRINT<<"ciao model viewer";
}

/////////////////////////////////////////////////////////////////

void FractureApp::updateProperty(const Property::ConstPtr &theProperty)
{
    ViewerApp::updateProperty(theProperty);
    
    if(theProperty == m_model_path)
    {
        gl::MeshPtr m = gl::AssimpConnector::loadModel(*m_model_path);
        
        if(m)
        {
            for(auto &t : m->material()->textures()){ textures().push_back(t); }
            
            scene().removeObject(m_mesh);
            m_physics.remove_mesh_from_simulation(m_mesh);
            m_mesh = m;
            
            auto aabb = m->boundingBox();
            float scale_factor = 50.f / aabb.width();
            m->setScale(scale_factor);
            
            scene().addObject(m_mesh);
            m_physics.add_mesh_to_simulation(m_mesh);
        }
    }
}

/////////////////////////////////////////////////////////////////

void FractureApp::shoot_box(const glm::vec3 &the_half_extents)
{
    gl::GeometryPtr geom = gl::Geometry::createBox(the_half_extents);
    auto box_shape = std::make_shared<btBoxShape>(physics::type_cast(the_half_extents));
    
    gl::MeshPtr mesh = gl::Mesh::create(geom, gl::Material::create());
    mesh->setPosition(camera()->position());
    scene().addObject(mesh);
    
    
    btRigidBody *rb = m_physics.add_mesh_to_simulation(mesh, pow(2 * the_half_extents.x, 3.f),
                                                       box_shape);
    rb->setLinearVelocity(physics::type_cast(camera()->lookAt() * 160.f));
    rb->setCcdSweptSphereRadius(1 / 2.f);
    rb->setCcdMotionThreshold(1 / 2.f);
}
