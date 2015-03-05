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
    
    registerProperty(m_view_type);
    registerProperty(m_model_path);
    registerProperty(m_texture_path);
    registerProperty(m_physics_running);
    registerProperty(m_physics_debug_draw);
    registerProperty(m_num_fracture_shards);
    registerProperty(m_breaking_thresh);
    registerProperty(m_gravity);
    registerProperty(m_friction);
    registerProperty(m_shoot_velocity);
    registerProperty(m_obj_scale);
    registerProperty(m_fbo_resolution);
    registerProperty(m_fbo_cam_pos);
    registerProperty(m_use_syphon);
    registerProperty(m_syphon_server_name);
    observeProperties();
    create_tweakbar_from_component(shared_from_this());
    
    m_light_component = std::make_shared<LightComponent>();
    m_light_component->set_lights(lights());
    create_tweakbar_from_component(m_light_component);
    
    // add lights to scene
    for (auto l : lights()){ scene().addObject(l ); }
    
    // init physics
    m_physics.init();
//    m_physics.set_world_boundaries(vec3(100), vec3(0, 100, 0));
    
    // box shooting stuff
    m_box_shape = std::make_shared<btBoxShape>(btVector3(.5f, .5f, .5f));
    m_box_geom = gl::Geometry::createBox(vec3(.5f));
    
    
    load_settings();
    m_light_component->refresh();
    
    fracture_test(*m_num_fracture_shards);
    
    m_gui_cam = gl::OrthographicCamera::create(0, gl::windowDimension().x, gl::windowDimension().y,
                                               0, 0, 1);
    
    // init joystick crosshairs
    m_crosshair_pos.resize(get_joystick_states().size());
    for(auto &p : m_crosshair_pos){ p = windowSize() / 2.f; }
}

/////////////////////////////////////////////////////////////////

void FractureApp::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    if(*m_physics_running){ m_physics.step_simulation(timeDelta); }
    
    // update joystick positions
    auto joystick_states = get_joystick_states();
    
    int i = 0;
    
    for(auto &joystick : joystick_states)
    {
        float min_val = .38f, multiplier = 400.f;
        float x_axis = abs(joystick.axis()[0]) > min_val ? joystick.axis()[0] : 0.f;
        float y_axis = abs(joystick.axis()[1]) > min_val ? joystick.axis()[1] : 0.f;
        m_crosshair_pos[i] += vec2(x_axis, y_axis) * multiplier * timeDelta;
        
        if(m_fbos[0])
        m_crosshair_pos[i] = glm::clamp(m_crosshair_pos[i], vec2(0), m_fbos[0].getSize());
        
        if(joystick.buttons()[0] && m_fbo_cam)
        {
            auto ray = gl::calculateRay(m_fbo_cam, m_crosshair_pos[i], m_fbos[0].getSize());
            shoot_box(ray, *m_shoot_velocity);
        }
        
        if(joystick.buttons()[9]){ fracture_test(*m_num_fracture_shards); break; }
        i++;
    }
}

/////////////////////////////////////////////////////////////////

void FractureApp::draw()
{
    // draw the output texture
    if(m_fbos[0] && m_fbo_cam)
    {
        auto tex = gl::render_to_texture(m_fbos[0],[&]
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            scene().render(m_fbo_cam);
            
            // gui stuff
            gl::setMatrices(m_gui_cam);
            for(auto &p : m_crosshair_pos)
            {
                gl::drawCircle(p, 15.f, false);
            }
            
        });
        textures()[1] = tex;
        
        if(*m_use_syphon){ m_syphon.publish_texture(tex); }
    }
    
    switch (*m_view_type)
    {
        case VIEW_DEBUG:
            gl::setMatrices(camera());
            if(draw_grid()){ gl::drawGrid(50, 50); }
            
            if(m_light_component->draw_light_dummies())
            {
                for (auto l : lights()){ gl::drawLight(l); }
            }
            
            if(*m_physics_debug_draw){ m_physics.debug_render(camera()); }
            else{ scene().render(camera()); }
            break;
            
        case VIEW_OUTPUT:
            gl::drawTexture(textures()[1], gl::windowDimension());
            break;
            
        default:
            break;
    }
    
    // draw texture map(s)
    if(displayTweakBar()){ draw_textures(); }
}

/////////////////////////////////////////////////////////////////

void FractureApp::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
    
    m_gui_cam = gl::OrthographicCamera::create(0, w, h, 0, 0, 1);
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
                fracture_test(*m_num_fracture_shards);
                break;
            case GLFW_KEY_P:
                *m_physics_running = !*m_physics_running;
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
        auto ray = gl::calculateRay(camera(), vec2(e.getX(), e.getY()));
        shoot_box(ray, *m_shoot_velocity);
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
                    textures() = {gl::createTextureFromFile(f, true, true, 4.f)};
                    
                    if(m_mesh)
                    {
                        m_mesh->material()->textures() = {textures().back()};
                    }
                    
                    *m_texture_path = f;
                }
                catch (Exception &e) { LOG_WARNING << e.what(); }
                if(scene().pick(gl::calculateRay(camera(), vec2(e.getX(), e.getY()))))
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
        addSearchPath(getDirectoryPart(*m_model_path));
        gl::MeshPtr m = gl::AssimpConnector::loadModel(*m_model_path);
        
        if(m)
        {
            for(auto &t : m->material()->textures()){ textures().push_back(t); }
            
            scene().removeObject(m_mesh);
            m_physics.remove_mesh_from_simulation(m_mesh);
            m_mesh = m;
            
            auto aabb = m->boundingBox();
            float scale_factor = 10.f / aabb.width();
            m->setScale(scale_factor);
            
            scene().addObject(m_mesh);
            m_physics.add_mesh_to_simulation(m_mesh);
        }
    }
    else if(theProperty == m_texture_path){}
    else if(theProperty == m_fbo_resolution)
    {
        gl::Fbo::Format fmt;
        fmt.setSamples(8);
        m_fbos[0] = gl::Fbo(m_fbo_resolution->value().x, m_fbo_resolution->value().y, fmt);
        float aspect = m_fbos[0].getAspectRatio();//m_obj_scale->value().x / m_obj_scale->value().y;
        m_fbo_cam = gl::PerspectiveCamera::create(aspect, 45.f, .1f, 100.f);
        m_fbo_cam->position() = *m_fbo_cam_pos;
    }
    else if(theProperty == m_fbo_cam_pos)
    {
        m_fbo_cam->position() = *m_fbo_cam_pos;
    }
    else if(theProperty == m_use_syphon)
    {
        m_syphon = *m_use_syphon ? syphon::Output(*m_syphon_server_name) : syphon::Output();
    }
    else if(theProperty == m_syphon_server_name)
    {
        try{m_syphon.setName(*m_syphon_server_name);}
        catch(syphon::SyphonNotRunningException &e){LOG_WARNING<<e.what();}
    }
    else if(theProperty == m_gravity)
    {
        if(m_physics.dynamicsWorld())
        {
            m_physics.dynamicsWorld()->setGravity(btVector3(0 , -1.f, 0) * *m_gravity);
        }
    }
    else if(theProperty == m_friction)
    {
        if(m_physics.dynamicsWorld())
        {
            for(int i = m_physics.dynamicsWorld()->getNumCollisionObjects() - 1; i >= 0; i--)
            {
                btRigidBody* b = btRigidBody::upcast(m_physics.dynamicsWorld()->getCollisionObjectArray()[i]);
                
                if(b){ b->setFriction(*m_friction); }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////

void FractureApp::shoot_box(const gl::Ray &the_ray, float the_velocity,
                            const glm::vec3 &the_half_extents)
{
//    auto box_shape = std::make_shared<btBoxShape>(physics::type_cast(the_half_extents));
    
    gl::MeshPtr mesh = gl::Mesh::create(m_box_geom, gl::Material::create());
    mesh->setScale(.2f * the_half_extents);
    mesh->setPosition(the_ray.origin);
    scene().addObject(mesh);
    m_box_shape->setLocalScaling(physics::type_cast(mesh->scale()));
    
    
    btRigidBody *rb = m_physics.add_mesh_to_simulation(mesh, pow(2 * the_half_extents.x, 3.f),
                                                       m_box_shape);
    rb->setFriction(*m_friction);
    rb->setLinearVelocity(physics::type_cast(the_ray.direction * the_velocity));
    rb->setCcdSweptSphereRadius(glm::length(mesh->scale() / 2.f));
    rb->setCcdMotionThreshold(glm::length(mesh->scale() / 2.f));
}

void FractureApp::fracture_test(uint32_t num_shards)
{
    scene().clear();
    m_physics.init();
    m_gravity->notifyObservers();
    
    auto phong_shader = gl::createShader(gl::SHADER_PHONG);
    
//    m_physics.set_world_boundaries(vec3(100), vec3(0, 100, 100 - .3f));
    btRigidBody *wall;
    {
        // ground plane
        auto ground_mat = gl::Material::create(phong_shader);
//        ground_mat->setDiffuse(gl::COLOR_BLACK);
        auto ground = gl::Mesh::create(gl::Geometry::createBox(vec3(.5f)), ground_mat);
        ground->setScale(vec3(100, .3, 100));
        auto ground_aabb = ground->boundingBox().transform(ground->transform());
        ground->position().y -= ground_aabb.halfExtents().y;
        auto col_shape = std::make_shared<btBoxShape>(physics::type_cast(ground_aabb.halfExtents()));
        btRigidBody *rb =m_physics.add_mesh_to_simulation(ground, 0.f, col_shape);
        rb->setFriction(*m_friction);
        scene().addObject(ground);
        
        // back plane
        auto back = gl::Mesh::create(gl::Geometry::createBox(vec3(.5f)), ground_mat);
        back->setScale(vec3(100, 20, .3));
        auto back_aabb = back->boundingBox().transform(back->transform());
        back->position() += vec3(0, back_aabb.halfExtents().y, -2.f * back_aabb.halfExtents().z);
        col_shape = std::make_shared<btBoxShape>(physics::type_cast(back_aabb.halfExtents()));
        wall = rb = m_physics.add_mesh_to_simulation(back, 0.f, col_shape);
        rb->setFriction(*m_friction);
        scene().addObject(back);
    }
    
    
    for(auto *b : m_physics.bounding_bodies()){ b->setFriction(*m_friction); }
    
    if(m_mesh)
    {
        scene().addObject(m_mesh);
        m_physics.add_mesh_to_simulation(m_mesh);
    }
    
    for(auto &l : lights()){ scene().addObject(l); }
    
    auto m = gl::Mesh::create(gl::Geometry::createBox(vec3(.5f)), gl::Material::create(phong_shader));
    m->setScale(*m_obj_scale);
    auto aabb = m->boundingBox().transform(m->transform());
    m->position().y += aabb.halfExtents().y;
    
    try
    {
        auto tex = gl::createTextureFromFile(*m_texture_path, true, true, 8.f);
        m->material()->addTexture(tex);
    }catch(Exception &e){ LOG_WARNING << e.what(); }
    
    // voronoi points
    std::vector<glm::vec3> voronoi_points;
    voronoi_points.resize(num_shards);
    for(auto &vp : voronoi_points)
    {
        vp = m->position() + glm::linearRand(-aabb.halfExtents(), aabb.halfExtents());
    }//(aabb.min, aabb.max); }
    
    Stopwatch t;
    t.start();
    
    m_voronoi_shards = physics::voronoi_convex_hull_shatter(m, voronoi_points);
    
//    m->position() += vec3(5, 0, 0);
//    scene().addObject(m);
//    m_physics.add_mesh_to_simulation(m);
    
    float density = 1.8;
    float convex_margin = 0.00;
    
    for(auto &s : m_voronoi_shards)
    {
        auto mesh_copy = s.mesh;//s.mesh->copy();
        scene().addObject(mesh_copy);
        s.mesh->material() = m->material();
        auto col_shape = physics::createConvexCollisionShape(mesh_copy);
        btRigidBody* rb = m_physics.add_mesh_to_simulation(mesh_copy, density * s.volume, col_shape);

        rb->getCollisionShape()->setMargin(convex_margin);
        rb->setRestitution(0.5f);
        rb->setFriction(*m_friction);
        rb->setRollingFriction(*m_friction);
        
        //ccd
        auto aabb = mesh_copy->boundingBox();
        rb->setCcdSweptSphereRadius(glm::length(aabb.halfExtents()) / 2.f);
        rb->setCcdMotionThreshold(glm::length(aabb.halfExtents()) / 2.f);
        
        // pin to wall
//        btTransform trA = rb->getWorldTransform(), trB = rb->getWorldTransform();
//        btVector3 pin_point = trA.getOrigin();
//        pin_point[2] = 0.f;//trB.getOrigin().z();
//        trB.setOrigin(pin_point);
//        btFixedConstraint* fixed = new btFixedConstraint(*rb, *wall, trA, trB);
//        fixed->setBreakingImpulseThreshold(*m_breaking_thresh);
//        fixed ->setOverrideNumSolverIterations(30);
//        m_physics.dynamicsWorld()->addConstraint(fixed,true);
        
    }
    m_physics.dynamicsWorld()->performDiscreteCollisionDetection();
    m_physics.attach_constraints(*m_breaking_thresh);
    
    LOG_DEBUG << "fracturing took " << t.time_elapsed() << " secs";
}
