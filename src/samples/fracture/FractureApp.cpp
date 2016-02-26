//
//  FractureApp.cpp
//  gl
//
//  Created by Fabian on 01/02/15.
//
//

#include "core/Timer.hpp"
#include "FractureApp.h"
#include "assimp/AssimpConnector.h"
#include "gl/ShaderLibrary.h"

using namespace std;
using namespace kinski;
using namespace glm;

/////////////////////////////////////////////////////////////////

void FractureApp::setup()
{
    ViewerApp::setup();
    
    m_texture_paths->setTweakable(false);
    
    register_property(m_view_type);
    register_property(m_model_path);
    register_property(m_crosshair_path);
    register_property(m_texture_paths);
    register_property(m_physics_running);
    register_property(m_physics_debug_draw);
    register_property(m_num_fracture_shards);
    register_property(m_breaking_thresh);
    register_property(m_gravity);
    register_property(m_friction);
    register_property(m_shoot_velocity);
    register_property(m_shots_per_sec);
    register_property(m_obj_scale);
    register_property(m_fbo_resolution);
    register_property(m_fbo_cam_pos);
    register_property(m_use_syphon);
    register_property(m_syphon_server_name);
    observe_properties();
    add_tweakbar_for_component(shared_from_this());
    
    m_light_component = std::make_shared<LightComponent>();
    m_light_component->set_lights(lights());
    add_tweakbar_for_component(m_light_component);
    
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
    
    m_gui_cam = gl::OrthographicCamera::create(0, gl::window_dimension().x, gl::window_dimension().y,
                                               0, 0, 1);
    
    // init joystick crosshairs
    m_crosshair_pos.resize(get_joystick_states().size());
    for(auto &p : m_crosshair_pos){ p = windowSize() / 2.f; }
    
    // dof material
//    gl::Shader sh; sh.loadFromData(unlit_vert, read_file("~/Desktop/shader_dof.frag").c_str());
//    m_dof_material = gl::Material::create(sh);
//    m_dof_material->setDepthTest(false);
//    m_dof_material->setDepthWrite(false);
//    m_dof_material->setBlending();
}

/////////////////////////////////////////////////////////////////

void FractureApp::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    m_time_since_last_shot += timeDelta;
    
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
        
        if(joystick.buttons()[0] && m_fbo_cam && m_time_since_last_shot > 1.f / *m_shots_per_sec)
        {
            auto ray = gl::calculate_ray(m_fbo_cam, m_crosshair_pos[i], m_fbos[0].getSize());
            shoot_box(ray, *m_shoot_velocity);
            m_time_since_last_shot = 0.f;
        }
        
        if(joystick.buttons()[9]){ fracture_test(*m_num_fracture_shards); break; }
        i++;
    }
    
    // movie updates
    if(m_movie && m_movie->copy_frame_to_texture(textures()[TEXTURE_INNER], true)){}
    
    if(m_crosshair_movie && m_crosshair_movie->copy_frame_to_texture(m_crosshair_tex, true)){}
}

/////////////////////////////////////////////////////////////////

void FractureApp::draw()
{
    // draw the output texture
    if(m_fbos[0] && m_fbo_cam && *m_use_syphon)
    {
        auto tex = gl::render_to_texture(m_fbos[0],[&]
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            scene().render(m_fbo_cam);
            
            // gui stuff
            gl::set_matrices(m_gui_cam);
            int crosshair_width = 70;
            
            for(auto &p : m_crosshair_pos)
            {
//                gl::drawCircle(p, 15.f, false);
                gl::draw_texture(m_crosshair_tex, vec2(crosshair_width), p - vec2(crosshair_width) / 2.f);
            }
            
        });
        textures()[TEXTURE_SYPHON] = tex;
        m_syphon.publish_texture(tex);
    }
    
    switch (*m_view_type)
    {
        case VIEW_DEBUG:
            gl::set_matrices(camera());
            if(draw_grid()){ gl::draw_grid(50, 50); }
            
            if(m_light_component->draw_light_dummies())
            {
                for (auto l : lights()){ gl::draw_light(l); }
            }
            
            if(*m_physics_debug_draw){ m_physics.debug_render(camera()); }
            else{ scene().render(camera()); }
            break;
            
        case VIEW_OUTPUT:
            gl::draw_texture(textures()[TEXTURE_SYPHON], gl::window_dimension());
//            m_dof_material->textures() = { textures()[TEXTURE_SYPHON], m_fbos[0].getDepthTexture() };
//            gl::drawQuad(m_dof_material, gl::windowDimension());
            break;
            
        default:
            break;
    }
    
    // draw texture map(s)
    if(displayTweakBar()){ draw_textures(textures()); }
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
        gl::CameraPtr cam = *m_view_type == VIEW_OUTPUT ? m_fbo_cam : camera();
        auto ray = gl::calculate_ray(cam, vec2(e.getX(), e.getY()));
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
    m_texture_paths->value().clear();
    
    for(const string &f : files)
    {
        LOG_INFO << f;

        // add path to searchpaths
        kinski::add_search_path(kinski::get_directory_part(f));
        m_search_paths->value().push_back(f);
        
        switch (get_file_type(f))
        {
            case FileType::MODEL:
                *m_model_path = f;
                break;
            
            case FileType::IMAGE:
            case FileType::MOVIE:
                m_texture_paths->value().push_back(f);
                if(scene().pick(gl::calculate_ray(camera(), vec2(e.getX(), e.getY()))))
                {
                    LOG_DEBUG << "texture drop on model";
                }
                break;
            default:
                break;
        }
    }
    m_texture_paths->notifyObservers();
}

/////////////////////////////////////////////////////////////////

void FractureApp::tearDown()
{
    LOG_PRINT<<"ciao model viewer";
}

/////////////////////////////////////////////////////////////////

void FractureApp::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
    
    if(theProperty == m_model_path)
    {
        add_search_path(get_directory_part(*m_model_path));
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
    else if(theProperty == m_crosshair_path)
    {
        auto ft = get_file_type(*m_crosshair_path);
        if(ft == FileType::MOVIE)
        {
            m_crosshair_movie = video::MovieController::create(*m_crosshair_path, true, true);
        }
        else if(ft == FileType::IMAGE)
        {
            m_crosshair_movie.reset();
            
            try
            {
                m_crosshair_tex  = gl::create_texture_from_file(*m_crosshair_path);
            } catch (FileNotFoundException &e) { LOG_WARNING << e.what(); }
        }
    }
    else if(theProperty == m_texture_paths)
    {
        std::vector<gl::Texture> tex_array;
        for(const string &f : m_texture_paths->value())
        {
            if(get_file_type(f) == FileType::IMAGE)
            {
                try{ tex_array.push_back(gl::create_texture_from_file(f, true, true, 8.f)); }
                catch (Exception &e) { LOG_WARNING << e.what(); }
            }else if(get_file_type(f) == FileType::MOVIE)
            {
                m_movie = video::MovieController::create(f, true, true);
            }
        }
        if(tex_array.size() > 0){ textures()[TEXTURE_OUTER] = tex_array[0]; }
        if(tex_array.size() > 1){ textures()[TEXTURE_INNER] = tex_array[1]; }
    }
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
    else if(theProperty == m_num_fracture_shards)
    {
        m_needs_refracture = true;
    }
}

/////////////////////////////////////////////////////////////////

void FractureApp::shoot_box(const gl::Ray &the_ray, float the_velocity,
                            const glm::vec3 &the_half_extents)
{
//    auto box_shape = std::make_shared<btBoxShape>(physics::type_cast(the_half_extents));
    static gl::Shader phong_shader;
    if(!phong_shader){ phong_shader = gl::create_shader(gl::ShaderType::PHONG); }
    gl::MeshPtr mesh = gl::Mesh::create(m_box_geom, gl::Material::create(phong_shader));
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
    
    auto phong_shader = gl::create_shader(gl::ShaderType::PHONG);
    
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
        
        // stopper
        auto stopper = gl::Mesh::create(gl::Geometry::createBox(vec3(.5f)), ground_mat);
        stopper->setScale(vec3(m_obj_scale->value().x, .1f, .1f));
        auto stopper_aabb = back->boundingBox().transform(back->transform());
        stopper->position() = vec3(0, 0,  - m_obj_scale->value().z / 2.f);
        col_shape = std::make_shared<btBoxShape>(physics::type_cast(stopper_aabb.halfExtents()));
        m_physics.add_mesh_to_simulation(stopper);
        
//        wall = rb = m_physics.add_mesh_to_simulation(back, 0.f, col_shape);
//        rb->setFriction(*m_friction);
//        scene().addObject(back);
        
        // wall left/right
//        m_physics.set_world_boundaries(vec3(m_obj_scale->value().x / 2.f, 100, 1000));
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
    
//    std::vector<gl::Texture> textures;
//    for(const auto &tex_path : m_texture_paths->value())
//    {
//        try
//        {
//            textures.push_back(gl::createTextureFromFile(tex_path, true, true, 8.f));
//        }catch(Exception &e){ LOG_WARNING << e.what(); }
//    }
//    m->material()->addTexture(tex);
    
    // voronoi points
    std::vector<glm::vec3> voronoi_points;
    voronoi_points.resize(num_shards);
    for(auto &vp : voronoi_points)
    {
        vp = m->position() + glm::linearRand(-aabb.halfExtents(), aabb.halfExtents());
    }//(aabb.min, aabb.max); }
    
    Stopwatch t;
    t.start();
    
    if(m_needs_refracture || m_voronoi_shards.empty())
    {
        m_voronoi_shards = physics::voronoi_convex_hull_shatter(m, voronoi_points);
        for(auto &s : m_voronoi_shards){ s.mesh->createVertexArray(); }
        m_needs_refracture = false;
    }
    
// original object without fracturing
//    m->position() += vec3(5, 0, 0);
//    scene().addObject(m);
//    m_physics.add_mesh_to_simulation(m);
    
    float density = 1.8;
    float convex_margin = 0.00;
    
    gl::MaterialPtr inner_mat(gl::Material::create()),
    outer_mat(gl::Material::create(phong_shader));
    
//    inner_mat->setDiffuse(gl::COLOR_RED);
    inner_mat->setSpecular(gl::COLOR_BLACK);
    
    outer_mat->textures() = { textures()[TEXTURE_OUTER] };
    inner_mat->textures() = { textures()[TEXTURE_INNER] };
    
    for(auto &s : m_voronoi_shards)
    {
        auto mesh_copy = s.mesh->copy();
        scene().addObject(mesh_copy);
        mesh_copy->materials() = {outer_mat, inner_mat};
        
        
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
