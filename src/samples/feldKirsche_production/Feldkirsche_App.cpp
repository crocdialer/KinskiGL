//
//  Feldkirsche_App.cpp
//  kinskiGL
//
//  Created by Fabian on 6/17/13.
//
//

#include "Feldkirsche_App.h"
#include <boost/asio/io_service.hpp>
#include "kinskiApp/ViewerApp.h"
#include "kinskiGL/Fbo.h"

// 3D model loading
#include "AssimpConnector.h"

// physics
#include "physics_context.h"

// Syphon
#include "SyphonConnector.h"

namespace kinski{
    
    using namespace std;
    using namespace kinski;
    using namespace glm;
    
    void Feldkirsche_App::setup()
    {
        ViewerApp::setup();
        set_precise_selection(true);
        
        /******************** add search paths ************************/
        kinski::addSearchPath("~/Desktop");
        kinski::addSearchPath("~/Desktop/Feldkirsche", true);
        kinski::addSearchPath("/Library/Fonts");
        m_font.load("Courier New Bold.ttf", 24);
        
        /*********** init our application properties ******************/
        
        m_stepPhysics = Property_<bool>::create("Step physics", true);
        registerProperty(m_stepPhysics);
        
        m_rigid_bodies_num = RangedProperty<int>::create("Num bodies", 1000, 0, 50000);
        registerProperty(m_rigid_bodies_num);
        
        m_rigid_bodies_size = RangedProperty<float>::create("Size of bodies", 20.f, .1f, 200.f);
        registerProperty(m_rigid_bodies_size);
        
        m_gravity = Property_<vec3>::create("Gravity", vec3(0, -1, 0));
        registerProperty(m_gravity);
        
        m_world_half_extents = Property_<vec3>::create("World dimensions", vec3(1500, 500, 200));
        registerProperty(m_world_half_extents);
        
        m_modelPath = Property_<string>::create("Model path", "duck.dae");
        registerProperty(m_modelPath);
        
        m_modelScale = Property_<glm::vec3>::create("Model scale", glm::vec3(1.f));
        registerProperty(m_modelScale);
        
        m_modelRotationY = Property_<float>::create("Model rotation Y", 0.f);
        registerProperty(m_modelRotationY);
        
        m_modelOffset = Property_<vec3>::create("Model Offset", vec3(0));
        registerProperty(m_modelOffset);
        
        m_color = Property_<glm::vec4>::create("Material color", glm::vec4(1 ,1 ,0, 0.6));
        registerProperty(m_color);
        
        m_shinyness = Property_<float>::create("Shinyness", 1.0);
        registerProperty(m_shinyness);
        
        m_debug_draw_mode = RangedProperty<int>::create("Debug draw mode", 0, 0, 1);
        registerProperty(m_debug_draw_mode);
        m_fbo_size = Property_<glm::vec2>::create("Fbo size", vec2(1024));
        registerProperty(m_fbo_size);
        m_fbo_cam_distance = RangedProperty<float>::create("Fbo cam distance", 200.f, 0.f, 10000.f);
        registerProperty(m_fbo_cam_distance);
        
        m_use_syphon = Property_<bool>::create("Use syphon", false);
        registerProperty(m_use_syphon);
        m_syphon_server_name = Property_<std::string>::create("Syphon server name", getName());
        registerProperty(m_syphon_server_name);
        
        m_fbo_cam_transform = Property_<glm::mat4>::create("FBO cam transform", mat4());
        m_fbo_cam_transform->setTweakable(false);
        registerProperty(m_fbo_cam_transform);
        
        create_tweakbar_from_component(shared_from_this());
        observeProperties();
        
        /////////////////////////////////// setup OpenNI interface /////////////////////////////////
        
        // OpenNI
        m_open_ni = gl::OpenNIConnector::Ptr(new gl::OpenNIConnector());
        
        // copy user colors
        m_user_id_colors = m_open_ni->user_colors();
        
        m_depth_cam_x = RangedProperty<float>::create("Depth_cam X", 0, -10000, 10000);
        m_open_ni->registerProperty(m_depth_cam_x);
        
        m_depth_cam_y = RangedProperty<float>::create("Depth_cam Y", 0, -10000, 10000);
        m_open_ni->registerProperty(m_depth_cam_y);
        
        m_depth_cam_z = RangedProperty<float>::create("Depth_cam Z", 0, -10000, 10000);
        m_open_ni->registerProperty(m_depth_cam_z);
        
        m_depth_cam_look_dir = Property_<vec3>::create("Depth_cam look dir", vec3(0, 0, -1));
        m_open_ni->registerProperty(m_depth_cam_look_dir);
        
        m_depth_cam_scale = RangedProperty<float>::create("Depth_cam scale", 1.f, .1f, 10.f);
        m_open_ni->registerProperty(m_depth_cam_scale);
        
        m_user_offset = RangedProperty<float>::create("User offset", 0.f, 0.f, 1000.f);
        m_open_ni->registerProperty(m_user_offset);
        
        m_min_interaction_distance = RangedProperty<float>::create("Min interaction distance",
                                                                   1200.f, 1, 2500);
        m_open_ni->registerProperty(m_min_interaction_distance);
        
        m_open_ni->observeProperties();
        create_tweakbar_from_component(m_open_ni);
        
        // add our app as open_ni observer
        observeProperties(m_open_ni->getPropertyList());
        
        /********************** construct a simple scene ***********************/
        camera()->setClippingPlanes(1.0, 15000);
        
        // the FBO camera
        m_free_camera = gl::PerspectiveCamera::Ptr(new gl::PerspectiveCamera(1.f, 45.f));
        
        // the virtual camera used to position depth camera input within the scene
        m_depth_cam = gl::PerspectiveCamera::Ptr(new gl::PerspectiveCamera(4/3.f, 45.f, 350.f, 4600.f));
        
        // Lights
        gl::LightPtr point_light(new gl::Light(gl::Light::POINT));
        point_light->setPosition(vec3(900, 600, 400));
        point_light->set_attenuation(0, .0005f, 0);
        lights().push_back(point_light);
        
        // setup some blank textures
//        m_textures.push_back(gl::Texture());
//        m_textures.push_back(gl::Texture());
        m_textures.resize(2);
        
        // clear with transparent black
        gl::clearColor(gl::Color(0));
        
        m_material = gl::Material::create(gl::createShader(gl::SHADER_PHONG));
        
        // init physics pipeline
        //m_physics_context = physics::physics_context(2);
        m_physics_context.initPhysics();
        m_debugDrawer.reset(new physics::BulletDebugDrawer);
        m_physics_context.dynamicsWorld()->setDebugDrawer(m_debugDrawer.get());
        
        // load state from config file
        try
        {
            Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
            Serializer::loadComponentState(m_open_ni, "ni_config.json", PropertyIO_GL());
        }catch(Exception &e)
        {
            LOG_WARNING << e.what();
        }
    }
    
    void Feldkirsche_App::update(float timeDelta)
    {
        ViewerApp::update(timeDelta);
        
        // update physics
        if (m_physics_context.dynamicsWorld() && *m_stepPhysics)
        {
            //m_physics_context.dynamicsWorld()->stepSimulation(timeDelta);
            auto task = boost::bind(&physics::physics_context::stepPhysics, &m_physics_context, timeDelta);
            //io_service().post(task);
            thread_pool().submit(task);
        }
        
        // update OpenNI
        if(m_open_ni->has_new_frame())
        {
            // query user positions from OpenNI (these are relative to depth_cam and Z inverted)
            m_user_list = m_open_ni->get_user_positions();
            adjust_user_positions_with_camera(m_user_list, m_depth_cam);
            
            // get the depth+userID texture
            m_textures[1] = m_open_ni->get_depth_texture();
        }
        
        // update gravity direction
        update_gravity(m_user_list, 0.05);
        
        // light update
        lights().front()->setPosition(light_direction());
        
        if(m_material)
        {
            m_material->setWireframe(wireframe());
            m_material->uniform("u_lightDir", light_direction());
            m_material->setDiffuse(m_color->value());
            m_material->setBlending(m_color->value().a < 1.0f);
            
        }
        for (int i = 0; i < materials().size(); i++)
        {
            materials()[i]->uniform("u_time",getApplicationTime());
            materials()[i]->uniform("u_lightDir", light_direction());
            materials()[i]->setShinyness(*m_shinyness);
            materials()[i]->setAmbient(0.2 * clear_color());
        }
    }
    
    void Feldkirsche_App::draw()
    {
        // draw block
        {
            if(m_fbo){m_textures[0] = gl::render_to_texture(scene(), m_fbo, m_free_camera);}
            
            switch(*m_debug_draw_mode)
            {
                case DRAW_DEBUG_SCENE:
                    gl::setMatrices(camera());
                    {
                        gl::clearColor(gl::Color(0, 0, 0, 1));
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                        if(draw_grid()){gl::drawGrid(3000, 3000, 20, 20);}
                        
                        if(wireframe())
                        {
                            m_physics_context.dynamicsWorld()->debugDrawWorld();
                            m_debugDrawer->flush();
                        }else
                        {
                            scene().render(camera());
                        }
                        m_debug_scene.render(camera());
                        gl::clearColor(clear_color());
                    }
                    m_debug_scene.render(camera());
                    draw_user_meshes(m_user_list);
                    break;
                    
                case DRAW_FBO_OUTPUT:
                    gl::drawTexture(m_textures[0], windowSize());
                    break;
                    
                default:
                    break;
            }
            
        }// FBO block
        
        if(*m_use_syphon)
        {
            m_syphon.publish_texture(m_textures[0]);
        }
        
        // draw texture map(s)
        if(displayTweakBar())
        {
            float w = (windowSize()/8.f).x;
            float h = m_textures[0].getHeight() * w / m_textures[0].getWidth();
            glm::vec2 offset(getWidth() - w - 10, 10);
            glm::vec2 step(0, h + 10);
            
            if(h > 0)
            {
                
                for (auto &tex : m_textures)
                {
                    float h = tex.getHeight() * w / tex.getWidth();
                    glm::vec2 step(0, h + 10);
                    drawTexture(tex, vec2(w, h), offset);
                    gl::drawText2D(as_string(tex.getWidth()) + std::string(" x ") +
                                   as_string(tex.getHeight()), m_font, glm::vec4(1),
                                   offset);
                    offset += step;
                }
            }
            
            // draw fps string
            gl::drawText2D(kinski::as_string(framesPerSec()), m_font,
                           vec4(vec3(1) - clear_color().xyz(), 1.f),
                           glm::vec2(windowSize().x - 110, windowSize().y - 70));
        }
    }
    
        
    void Feldkirsche_App::mousePress(const MouseEvent &e)
    {
        ViewerApp::mousePress(e);
    }
    
    void Feldkirsche_App::keyPress(const KeyEvent &e)
    {
        ViewerApp::keyPress(e);
        int min, max;
        
        if(!e.isShiftDown() && !e.isAltDown())
        {
            //gl::Visitor visitor;
            
            switch (e.getCode())
            {
                case GLFW_KEY_W:
                    set_wireframe(!wireframe());
                    break;
                    
                case GLFW_KEY_I:
                    //scene().root()->accept(visitor);
                    break;
                    
                case GLFW_KEY_D:
                    m_debug_draw_mode->getRange(min, max);
                    *m_debug_draw_mode = (*m_debug_draw_mode + 1) % (max + 1);
                    break;
                    
                case GLFW_KEY_P:
                    *m_stepPhysics = !*m_stepPhysics;
                    break;
                    
                case GLFW_KEY_R:
                    Serializer::loadComponentState(m_open_ni, "ni_config.json", PropertyIO_GL());
                    m_rigid_bodies_num->set(*m_rigid_bodies_num);
                    break;
                    
                case GLFW_KEY_S:
                    Serializer::saveComponentState(m_open_ni, "ni_config.json", PropertyIO_GL());
                    break;

                case GLFW_KEY_UP:
                    LOG_DEBUG<<"TILT UP";
                    m_ground_body->getWorldTransform().setOrigin(btVector3(0, 50, 0));
                    break;
                    
                case GLFW_KEY_LEFT:
                    LOG_DEBUG<<"TILT LEFT";
                    m_left_body->getWorldTransform().setOrigin(btVector3(60, 0, 0));
                    break;
                    
                case GLFW_KEY_RIGHT:
                    LOG_DEBUG<<"TILT RIGHT";
                    m_right_body->getWorldTransform().setOrigin(btVector3(-60, 0, 0));
                    break;
                    
                default:
                    break;
            }
        }
        
        btTransform trans = btTransform::getIdentity();
        
        if(e.isShiftDown())
        {
            switch (e.getCode())
            {
                case GLFW_KEY_LEFT:
                    LOG_DEBUG<<"ROLL LEFT";
                    trans.setRotation(btQuaternion(0.f, 0.f, glm::radians(10.f)));
                    m_ground_body->setWorldTransform(trans);
                    break;
                    
                case GLFW_KEY_RIGHT:
                    LOG_DEBUG<<"ROLL RIGHT";
                    trans.setRotation(btQuaternion(0.f, 0.f, glm::radians(-10.f)));
                    m_ground_body->setWorldTransform(trans);
                    break;
                    
                default:
                    break;
            }
        }
        
        if(e.isAltDown())
        {
            float step_size = 10.f;
            
            switch (e.getCode())
            {
                case GLFW_KEY_LEFT:
                    *m_fbo_cam_transform = glm::translate(m_fbo_cam_transform->value(),
                                                          step_size * glm::vec3(1, 0, 0));
                    break;
                    
                case GLFW_KEY_RIGHT:
                    *m_fbo_cam_transform = glm::translate(m_fbo_cam_transform->value(),
                                                          step_size * glm::vec3(-1, 0, 0));
                    break;
                    
                case GLFW_KEY_UP:
                    *m_fbo_cam_transform = glm::translate(m_fbo_cam_transform->value(),
                                                          step_size * glm::vec3(0, -1, 0));
                    break;
                    
                case GLFW_KEY_DOWN:
                    *m_fbo_cam_transform = glm::translate(m_fbo_cam_transform->value(),
                                                          step_size * glm::vec3(0, 1, 0));
                    break;
                    
                default:
                    break;
            }
        }
    }
    
    void Feldkirsche_App::keyRelease(const KeyEvent &e)
    {
        ViewerApp::keyRelease(e);
        
        switch (e.getCode())
        {
            case GLFW_KEY_UP:
                m_ground_body->setWorldTransform(btTransform::getIdentity());
                break;
                
            case GLFW_KEY_LEFT:
                m_left_body->getWorldTransform().setOrigin(btVector3(0, 0, 0));
                break;
                
            case GLFW_KEY_RIGHT:
                m_right_body->getWorldTransform().setOrigin(btVector3(0, 0, 0));
                break;
        }
    }
    
    // Property observer callback
    void Feldkirsche_App::updateProperty(const Property::ConstPtr &theProperty)
    {
        ViewerApp::updateProperty(theProperty);
        
        // one of our porperties was changed
        if(theProperty == m_color)
        {
            if(selected_mesh()) selected_mesh()->material()->setDiffuse(*m_color);
        }
        else if(theProperty == m_shinyness)
        {
            if(m_mesh) m_mesh->material()->setShinyness(*m_shinyness);
        }
        else if(theProperty == m_modelPath || theProperty == m_modelScale ||
                theProperty == m_modelOffset || theProperty == m_modelRotationY)
        {
            scene().removeObject(m_mesh);
            materials().clear();
            
            try
            {
                gl::MeshPtr m = gl::AssimpConnector::loadModel(*m_modelPath);
                scene().removeObject(m_mesh);
                
                // reset physics scene
                m_rigid_bodies_num->set(*m_rigid_bodies_num);
                
                add_mesh(m, *m_modelScale);
            }
            catch (Exception &e)
            {
                // check if modelpath is a folder
                if(kinski::isDirectory(*m_modelPath))
                {
                    //TODO load all models
                    try
                    {
                        std::list<string> models = kinski::getDirectoryEntries(*m_modelPath, false, "dae");
                        for (const auto &model : models)
                        {
                            add_mesh(gl::AssimpConnector::loadModel(model), *m_modelScale);
                        }
                        
                    }catch(Exception &e){LOG_ERROR<< e.what();}
                }
            }
        }
        else if(theProperty == m_fbo_size || theProperty == m_fbo_cam_distance)
        {
            m_fbo = gl::Fbo(m_fbo_size->value().x, m_fbo_size->value().y);
            m_free_camera->setAspectRatio(m_fbo_size->value().x / m_fbo_size->value().y);
            
            mat4 m = m_fbo_cam_transform->value();
            m[3].z = *m_fbo_cam_distance;
            m_fbo_cam_transform->set(m);
        }
        else if(theProperty == m_fbo_cam_transform)
        {
            m_free_camera->setTransform(*m_fbo_cam_transform);
            m_debug_scene.removeObject(m_free_camera_mesh);
            m_free_camera_mesh = gl::createFrustumMesh(m_free_camera);
            m_debug_scene.addObject(m_free_camera_mesh);
        }
        else if(theProperty == m_gravity)
        {
            if(m_physics_context.dynamicsWorld())
            {
                vec3 gravity_vec = glm::normalize(m_gravity->value()) * m_physics_context.dynamicsWorld()->getGravity().length();
                m_physics_context.dynamicsWorld()->setGravity(physics::type_cast(gravity_vec));
            }
        }
        else if(theProperty == m_rigid_bodies_num || theProperty == m_rigid_bodies_size)
        {
            int num_xy = floor(sqrt(*m_rigid_bodies_num / 2.f));
            create_physics_scene(num_xy, num_xy, 2, m_material);
            //add_mesh(m_mesh, *m_modelScale);
        }
        else if(theProperty == m_use_syphon)
        {
            m_syphon = *m_use_syphon ? gl::SyphonConnector(*m_syphon_server_name) : gl::SyphonConnector();
        }
        else if(theProperty == m_syphon_server_name)
        {
            try{m_syphon.setName(*m_syphon_server_name);}
            catch(gl::SyphonNotRunningException &e){LOG_WARNING<<e.what();}
        }
        else if(theProperty == m_depth_cam_x || theProperty == m_depth_cam_y ||
                theProperty == m_depth_cam_z || theProperty == m_depth_cam_look_dir ||
                theProperty == m_depth_cam_scale)
        {
            m_depth_cam->setPosition(vec3(m_depth_cam_x->value(), m_depth_cam_y->value(), m_depth_cam_z->value()));
            m_depth_cam->setLookAt(m_depth_cam->position() + m_depth_cam_look_dir->value() );
            m_depth_cam->setTransform(glm::scale(m_depth_cam->transform(), vec3(*m_depth_cam_scale)));
            
            m_debug_scene.removeObject(m_depth_cam_mesh);
            m_depth_cam_mesh = gl::createFrustumMesh(m_depth_cam);
            m_depth_cam_mesh->material()->setDiffuse(gl::Color(1, 0, 0, 1));
            m_debug_scene.addObject(m_depth_cam_mesh);
        }
    }
    
    void Feldkirsche_App::tearDown()
    {
        LOG_DEBUG<<"waiting for OpenNI to shut down";
        if(m_open_ni) m_open_ni->stop();
        LOG_INFO<<"ciao Feldkirsche";
    }
    
    void Feldkirsche_App::add_mesh(const gl::MeshPtr &the_mesh, vec3 scale)
    {
        if(!the_mesh) return;
        
        m_mesh = the_mesh;
        materials().push_back(the_mesh->material());
        the_mesh->material()->setShinyness(*m_shinyness);
        the_mesh->material()->setSpecular(glm::vec4(1));
        the_mesh->transform() *= glm::rotate(glm::mat4(), m_modelRotationY->value(),
                                             vec3(0, 1, 0));
        the_mesh->setPosition(the_mesh->position() - vec3(0, the_mesh->boundingBox().min.y, 0));
        the_mesh->position() += m_modelOffset->value();
        scene().addObject(m_mesh);
        
        physics::btCollisionShapePtr customShape = physics::createCollisionShape(the_mesh, scale);
        m_physics_context.collisionShapes().push_back(customShape);
        physics::MotionState *ms = new physics::MotionState(m_mesh);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(0.f, ms, customShape.get());
        btRigidBody* body = new btRigidBody(rbInfo);
        body->setFriction(2.f);
        body->setCollisionFlags( body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        body->setActivationState(DISABLE_DEACTIVATION);
        
        //add the body to the dynamics world
        m_physics_context.dynamicsWorld()->addRigidBody(body);
        
        m_mesh->transform() *= glm::scale(glm::mat4(), scale);
    }
    
    void Feldkirsche_App::create_physics_scene(int size_x, int size_y, int size_z, const gl::MaterialPtr &theMat)
    {
        scene().clear();
        m_physics_context.teardown_physics();
        m_physics_context.dynamicsWorld()->setGravity(btVector3(0, -981.f, 0));
        
        float scaling = *m_rigid_bodies_size;
        float start_pox_x = -5;
        float start_pox_y = -5;
        float start_pox_z = -3;
        
        // add static plane boundaries
        physics::btCollisionShapePtr ground_plane (new btStaticPlaneShape(btVector3(0, 1, 0), 0)),
        front_plane(new btStaticPlaneShape(btVector3(0, 0, -1),-150)),
        back_plane(new btStaticPlaneShape(btVector3(0, 0, 1), -150)),
        left_plane(new btStaticPlaneShape(btVector3(1, 0, 0),- m_world_half_extents->value()[0])),
        right_plane(new btStaticPlaneShape(btVector3(-1, 0, 0), - m_world_half_extents->value()[0])),
        top_plane(new btStaticPlaneShape(btVector3(0, -1, 0), - m_world_half_extents->value()[1]));
        m_physics_context.collisionShapes().push_back(ground_plane);
        m_physics_context.collisionShapes().push_back(front_plane);
        m_physics_context.collisionShapes().push_back(back_plane);
        m_physics_context.collisionShapes().push_back(left_plane);
        m_physics_context.collisionShapes().push_back(right_plane);
        m_physics_context.collisionShapes().push_back(top_plane);
        
        for (int i = 0; i < m_physics_context.collisionShapes().size(); ++i)
        {
            //gl::MotionState* myMotionState = new gl::MotionState(plane_mesh);
            btRigidBody::btRigidBodyConstructionInfo rbInfo(0.f,
                                                            NULL,
                                                            m_physics_context.collisionShapes()[i].get());
            btRigidBody* body = new btRigidBody(rbInfo);
            body->setFriction(2.f);
            body->setCollisionFlags( body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
            body->setActivationState(DISABLE_DEACTIVATION);
            
            //add the body to the dynamics world
            m_physics_context.dynamicsWorld()->addRigidBody(body);
        }
        
        // assign to members
        m_ground_body = (btRigidBody*) m_physics_context.dynamicsWorld()->getCollisionObjectArray()[0];
        m_left_body = (btRigidBody*) m_physics_context.dynamicsWorld()->getCollisionObjectArray()[3];
        m_right_body = (btRigidBody*) m_physics_context.dynamicsWorld()->getCollisionObjectArray()[4];
        
        {
            //create a few dynamic rigidbodies
            // Re-using the same collision is better for memory usage and performance
            
            m_physics_context.collisionShapes().push_back(physics::btCollisionShapePtr(new btSphereShape(scaling)));
            
            // Create Dynamic Objects
            btTransform startTransform;
            startTransform.setIdentity();
            btScalar	mass(1.f);
            
            // rigidbody is dynamic if and only if mass is non zero, otherwise static
            bool isDynamic = (mass != 0.f);
            
            btVector3 localInertia(0,0,0);
            if (isDynamic)
                m_physics_context.collisionShapes().back()->calculateLocalInertia(mass,localInertia);
            
            // geometry
            gl::Geometry::Ptr geom = gl::createSphere(scaling, 32);
            
            float start_x = start_pox_x - size_x/2;
            float start_y = start_pox_y;
            float start_z = start_pox_z - size_z/2;
            
            for (int k = 0; k < size_y; k++)
            {
                for (int i = 0; i < size_x; i++)
                {
                    for(int j = 0; j < size_z; j++)
                    {
                        startTransform.setOrigin(scaling * btVector3(btScalar(2.0*i + start_x),
                                                                     btScalar(20+2.0*k + start_y),
                                                                     btScalar(2.0*j + start_z)));
                        gl::MeshPtr mesh = gl::Mesh::create(geom, theMat);
                        scene().addObject(mesh);
                        float mat[16];
                        startTransform.getOpenGLMatrix(mat);
                        mesh->setTransform(glm::make_mat4(mat));
                        
                        //using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
                        physics::MotionState* myMotionState = new physics::MotionState(mesh);
                        
                        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,
                                                                        m_physics_context.collisionShapes().back().get(),
                                                                        localInertia);
                        btRigidBody* body = new btRigidBody(rbInfo);
                        //body->setFriction(2.f);
                        //body->setDamping(0.f, 2.f);
                        body->setCcdMotionThreshold(scaling * scaling);
                        body->setCcdSweptSphereRadius(scaling / 4);
                        m_physics_context.dynamicsWorld()->addRigidBody(body);
                    }
                }
            }
        }
        LOG_DEBUG<<"created dynamicsworld with "<<
        m_physics_context.dynamicsWorld()->getNumCollisionObjects()<<" rigidbodies";
        
        // add our lights
        for (auto &light : lights()){scene().addObject(light);}
    }
    
    //! bring positions to world-coords using a virtual camera
    void Feldkirsche_App::adjust_user_positions_with_camera(gl::OpenNIConnector::UserList &user_list,
                                                            const gl::CameraPtr &cam)
    {
        for(auto &user : user_list)
        {
            vec4 flipped_pos (user.position, 1.f);flipped_pos.z *= - 1.0f;
            user.position = (cam->transform() * flipped_pos).xyz();
        }
    }
    
    void Feldkirsche_App::draw_user_meshes(const gl::OpenNIConnector::UserList &user_list)
    {
        if(!m_user_mesh)
        {
            m_user_mesh = gl::Mesh::create(gl::createSphere(1.f, 32), gl::Material::create());
            m_user_mesh->transform() *= glm::scale(glm::mat4(), glm::vec3(100));
            m_user_mesh->transform() *= glm::rotate(glm::mat4(), -90.f, glm::vec3(1, 0, 0));
        }
        if(!m_user_radius_mesh)
        {
            m_user_radius_mesh = gl::Mesh::create(gl::createUnitCircle(64), gl::Material::create());
            m_user_radius_mesh->transform() *= glm::rotate(glm::mat4(), -90.f, glm::vec3(1, 0, 0));
        }
        gl::loadMatrix(gl::PROJECTION_MATRIX, camera()->getProjectionMatrix());
        
        for (const auto &user : user_list)
        {
            m_user_mesh->setPosition(user.position);
            m_user_mesh->material()->setDiffuse(m_user_id_colors[user.id]);
            gl::loadMatrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix() * m_user_mesh->transform());
            gl::drawMesh(m_user_mesh);
            
            m_user_radius_mesh->setPosition(user.position - vec3(0, 0, 1) * m_user_offset->value());
            m_user_radius_mesh->material()->setDiffuse(gl::Color(1, 0, 0, 1));
            gl::loadMatrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix() *
                           m_user_radius_mesh->transform() * glm::scale(glm::mat4(),
                                                                        glm::vec3(*m_min_interaction_distance)));
            gl::drawMesh(m_user_radius_mesh);
        }
    }
    
    void Feldkirsche_App::update_gravity(const gl::OpenNIConnector::UserList &user_list, float factor)
    {
        glm::vec3 direction_avg(0, -1.f, 0);
        
        if(!user_list.empty())
        {
            direction_avg = glm::vec3(0);

            for (const auto &user : user_list)
            {
                direction_avg += glm::normalize(user.position);
            }
        
            // kill z-component and assure downward gravity
            direction_avg.z = 0;
            direction_avg.y = std::min(direction_avg.y, -0.5f);
        }
        // use factor for mixing new and current directions
        *m_gravity = glm::mix(m_gravity->value(), glm::normalize(direction_avg), factor);
    }
}