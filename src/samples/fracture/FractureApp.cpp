//
//  FractureApp.cpp
//  gl
//
//  Created by Fabian on 01/02/15.
//
//

#include "core/Timer.h"
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
    m_physics.set_world_boundaries(vec3(100), vec3(0, 100, 0));
    
    // box shooting stuff
    m_box_shape = std::make_shared<btBoxShape>(btVector3(.5f, .5f, .5f));
    m_box_geom = gl::Geometry::createBox(vec3(.5f));
    
    
    load_settings();
    m_light_component->refresh();
    
    fracture_test();
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
    
    if(!displayTweakBar())
    {
        switch (e.getCode())
        {
            case GLFW_KEY_V:
                fracture_test();
                break;
                
            default:
                break;
        }
    }
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
        shoot_box(ray, 160.f);
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
                    LOG_DEBUG << "texture drop on model";
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

void FractureApp::shoot_box(const gl::Ray &the_ray, float the_velocity,
                            const glm::vec3 &the_half_extents)
{
//    auto box_shape = std::make_shared<btBoxShape>(physics::type_cast(the_half_extents));
    
    gl::MeshPtr mesh = gl::Mesh::create(m_box_geom, gl::Material::create());
    mesh->setScale(2.f * the_half_extents);
    mesh->setPosition(the_ray.origin);
    scene().addObject(mesh);
    
    
    btRigidBody *rb = m_physics.add_mesh_to_simulation(mesh, pow(2 * the_half_extents.x, 3.f),
                                                       m_box_shape);
    rb->setLinearVelocity(physics::type_cast(the_ray.direction * the_velocity));
    rb->setCcdSweptSphereRadius(1 / 2.f);
    rb->setCcdMotionThreshold(1 / 2.f);
}

void FractureApp::fracture_test()
{
    Stopwatch t;
    t.start();
    
    scene().clear();
    
    auto m = gl::Mesh::create(gl::Geometry::createSphere(.5f, 8), gl::Material::create());
    //    m->setPosition(vec3(0, 500, 0));
    auto aabb = m->boundingBox().transform(m->transform());
    
    // voronoi points
    std::vector<glm::vec3> voronoi_points;
    voronoi_points.resize(10);
    for(auto &vp : voronoi_points){ vp = glm::linearRand(aabb.min, aabb.max); }
    
    auto phong_shader = gl::createShader(gl::SHADER_PHONG);
    auto sr = physics::voronoi_convex_hull_shatter(voronoi_points, m, 3.f);
    
    
    for(auto &sm : sr.shard_meshes)
    {
        scene().addObject(sm);
        
//        sm->material()->setShader(phong_shader);
//        sm->material()->setWireframe();
        sm->material()->setDiffuse(glm::linearRand(gl::COLOR_GREEN, gl::COLOR_WHITE));
    }
    
    LOG_DEBUG << "fracturing took " << t.time_elapsed() << "secs";
}
