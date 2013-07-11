//
//  Feldkirsche_App.cpp
//  kinskiGL
//
//  Created by Fabian on 6/17/13.
//
//

#include "Feldkirsche_App.h"
#include "kinskiCV/TextureIO.h"

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
        
        m_custom_shader_paths = Property_<std::vector<std::string> >::create("Custom shader paths",
                                                                             std::vector<std::string>());
        m_custom_shader_paths->setTweakable(false);
        registerProperty(m_custom_shader_paths);
        
        m_stepPhysics = Property_<bool>::create("Step physics", true);
        registerProperty(m_stepPhysics);
        
        m_rigid_bodies_num = RangedProperty<int>::create("Num bodies", 1000, 0, 50000);
        registerProperty(m_rigid_bodies_num);
        
        m_rigid_bodies_size = RangedProperty<float>::create("Size of bodies", 20.f, .1f, 200.f);
        registerProperty(m_rigid_bodies_size);
        
        m_gravity = Property_<vec3>::create("Gravity", vec3(0, -1, 0));
        registerProperty(m_gravity);
        
        m_gravity_amount = Property_<float>::create("Gravity amount", -987.f);
        registerProperty(m_gravity_amount);
        
        m_gravity_max_roll = RangedProperty<float>::create("Gravity max roll", 0.2, 0, 1);
        registerProperty(m_gravity_max_roll);
        
        m_gravity_smooth = RangedProperty<float>::create("Gravity smooth", 0.03, 0.f, 1.f);
        registerProperty(m_gravity_smooth);
        
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
        
        m_reflection = RangedProperty<float>::create("Reflection", .5, 0, 1);
        registerProperty(m_reflection);
        
        m_debug_draw_mode = RangedProperty<int>::create("Debug draw mode", 0, 0, 1);
        registerProperty(m_debug_draw_mode);
        
        m_fbo_size = Property_<glm::vec2>::create("Fbo size", vec2(1024));
        registerProperty(m_fbo_size);
        
        m_fbo_cam_distance = RangedProperty<float>::create("Fbo cam distance", 200.f, 0.f, 10000.f);
        registerProperty(m_fbo_cam_distance);
        
        m_fbo_cam_direction = Property_<vec3>::create("Fbo cam lookat", - gl::Z_AXIS);
        registerProperty(m_fbo_cam_direction);
        
        m_fbo_cam_fov = RangedProperty<float>::create("Fbo cam FOV", 30.f, 1.f, 90.f);
        registerProperty(m_fbo_cam_fov);
        
        m_use_syphon = Property_<bool>::create("Use syphon", false);
        registerProperty(m_use_syphon);
        m_syphon_server_name = Property_<std::string>::create("Syphon server name", getName());
        registerProperty(m_syphon_server_name);
        
        m_fbo_cam_transform = Property_<glm::mat4>::create("FBO cam transform", mat4());
        m_fbo_cam_transform->setTweakable(false);
        registerProperty(m_fbo_cam_transform);

        create_tweakbar_from_component(shared_from_this());
        observeProperties();
        
        
        // light component
        m_light_component.reset(new LightComponent());
        create_tweakbar_from_component(m_light_component);
        
        /********************** construct a simple scene ***********************/
        camera()->setClippingPlanes(1.0, 15000);
        
        // the FBO camera
        m_free_camera = gl::PerspectiveCamera::Ptr(new gl::PerspectiveCamera(1.f, *m_fbo_cam_fov, 10.f, 10000));
        
        
        // Lights
        m_spot_light = lights()[1];
        m_spot_light->set_type(gl::Light::SPOT);
        m_spot_light->setPosition(vec3(-300, 1100, 450));
        m_spot_light->setLookAt(vec3(200, 0, 0));
        m_spot_light->set_attenuation(0, .0006f, 0);
        //spot_light->set_specular(gl::Color(0.4));
        m_spot_light->set_spot_cutoff(70.f);
        m_spot_light->set_spot_exponent(10.f);
        
        // lightcone
        gl::Geometry::Ptr cone_geom(gl::createCone(150, 200, 32));
        gl::MeshPtr cone_mesh = gl::Mesh::create(cone_geom, gl::Material::create());
        cone_mesh->material()->setDiffuse(gl::Color(1.f, 1.f, .9f, .7f));
        cone_mesh->material()->setBlending();
        cone_mesh->material()->setDepthWrite(false);
        cone_mesh->transform() = glm::rotate(cone_mesh->transform(), 90.f, gl::X_AXIS);
        cone_mesh->position() += glm::vec3(0, 0, -200);
        
        // lighthouse spot
        m_spot_mesh = gl::Mesh::create(gl::createCone(150, 700, 32), gl::Material::create());
        for(auto &vertex : m_spot_mesh->geometry()->vertices()){vertex.y -= 700;}
        m_spot_mesh->geometry()->colors().resize(m_spot_mesh->geometry()->vertices().size());
        
        for (int i = 0; i < m_spot_mesh->geometry()->vertices().size(); i++)
        {
            m_spot_mesh->geometry()->colors()[i] = gl::Color(1, 0, 0, 1 - (m_spot_mesh->geometry()->vertices()[i].y / (-700.f))) ;
        }
        m_spot_mesh->geometry()->createGLBuffers();
        m_spot_mesh->geometry()->computeBoundingBox();
        m_spot_mesh->material()->setDiffuse(gl::Color(1.f, 1.f, .9f, .9f));
        m_spot_mesh->material()->setBlending();
        m_spot_mesh->material()->setDepthWrite(true);
        m_spot_mesh->transform() = glm::rotate(m_spot_mesh->transform(), 90.f, gl::X_AXIS);
        m_spot_mesh->position() = vec3(0, 490, 0);
        
        // test spot
//        gl::MeshPtr spot_mesh = gl::Mesh::create(gl::createSphere(10.f, 32), gl::Material::create());
//        m_spot_light->add_child(spot_mesh);
//        m_spot_light->add_child(cone_mesh);
        
        m_light_component->set_lights(lights());
        
        // setup some blank textures
        for(int i = 0; i < 4; i++){m_textures.push_back(gl::Texture());}
        
        // background image
        //m_textures[3] = gl::createTextureFromFile("BG_sunset_02.jpg", true, true);
        
        // clear with transparent black
        gl::clearColor(gl::Color(0));
        
        m_material = gl::Material::create(gl::createShader(gl::SHADER_PHONG));

        m_physics_context.initPhysics();
        m_debugDrawer.reset(new physics::BulletDebugDrawer);
        m_physics_context.dynamicsWorld()->setDebugDrawer(m_debugDrawer.get());
        
        // load state from config file(s)
        load_settings();
    }
    
    void Feldkirsche_App::save_settings(const std::string &path)
    {
        m_gravity->removeObserver(shared_from_this());
        *m_gravity = vec3(0, -1, 0);
        ViewerApp::save_settings(path);
        m_gravity->addObserver(shared_from_this());
    }
    
    void Feldkirsche_App::load_settings(const std::string &path)
    {
        ViewerApp::load_settings(path);
    }
    
    void Feldkirsche_App::update(float timeDelta)
    {
        timeDelta = 1 / 30.f;
        
        ViewerApp::update(timeDelta);
        
        // update physics
        if (m_physics_context.dynamicsWorld() && *m_stepPhysics)
        {
            m_physics_context.dynamicsWorld()->stepSimulation(timeDelta);
//            auto task = boost::bind(&physics::physics_context::stepPhysics, &m_physics_context, timeDelta);
//            thread_pool().submit(task);
        }
        
        // light update
        //m_light_component->set_lights(lights());
        
        // rotate lighthouse cone
        m_spot_mesh->transform() = glm::rotate(m_spot_mesh->transform(), 50.f * timeDelta, gl::Z_AXIS);
        
        m_material->setWireframe(wireframe());
        m_material->setDiffuse(m_color->value());
        m_material->setBlending(m_color->value().a < 1.0f);
        m_material->uniform("u_time",getApplicationTime());
        m_material->setShinyness(*m_shinyness);
        
        m_material->setAmbient(0.2 * clear_color());
        
    }
    
    void Feldkirsche_App::draw()
    {
        // draw block
        {
            if(m_fbo){m_textures[0] = gl::render_to_texture(scene(), m_fbo, m_free_camera);}
            
            // calc texture sizes
            float aspect = m_textures[0].getWidth() / (float)m_textures[0].getHeight();
            float w, h;
            w = windowSize().y * aspect;
            h = windowSize().y;
            vec2 offset( (windowSize().x - w) / 2.f, 0);
            
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
                    break;
                    
                case DRAW_FBO_OUTPUT:
                    // background
                    //gl::drawTexture(m_textures[3], windowSize());
                    gl::drawTexture(m_textures[0], vec2(w, h), offset);
                    break;
                    
                default:
                    break;
            }
            
        }// FBO block
        
        if(*m_use_syphon)
        {
            //m_syphon.publish_texture(m_textures[0]);
            
            // image sequence
            {
                static int frame_count = 0;
                static char buf[512];
                sprintf(buf, "/Users/Fabian/Desktop/image_sequence_00/feldkirsche_%04i.png", frame_count);
                gl::TextureIO::saveTexture(buf, m_textures[0], true);
                frame_count++;
                
                if(!(frame_count % 30)){LOG_INFO<<frame_count / 30<<" sec";}
            }
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
                           vec4(vec3(1), 1.f),
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

                case GLFW_KEY_UP:
                    LOG_DEBUG<<"TILT UP";
                    m_ground_body->getWorldTransform().setOrigin(btVector3(0, 60, 0));
                    break;
                    
                case GLFW_KEY_LEFT:
                    LOG_DEBUG<<"TILT LEFT";
                    m_left_body->getWorldTransform().setOrigin(btVector3(55, 0, 0));
                    break;
                    
                case GLFW_KEY_RIGHT:
                    LOG_DEBUG<<"TILT RIGHT";
                    m_right_body->getWorldTransform().setOrigin(btVector3(-55, 0, 0));
                    break;
                    
                default:
                    break;
            }
        }

        if(e.isShiftDown())
        {
            switch (e.getCode())
            {
                case GLFW_KEY_LEFT:
                    LOG_DEBUG<<"ROLL LEFT";
                    m_mesh->transform() = glm::rotate(m_mesh->transform(), .5f, glm::vec3(0, 0, 1));
                    break;
                    
                case GLFW_KEY_RIGHT:
                    LOG_DEBUG<<"ROLL RIGHT";
                    m_mesh->transform() = glm::rotate(m_mesh->transform(), -.5f, glm::vec3(0, 0, 1));
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
                m->material()->setSpecular(gl::Color(1));
                m->material()->setShinyness(*m_shinyness);
                
                scene().removeObject(m_mesh);
                
                // reset physics scene
                m_rigid_bodies_num->set(*m_rigid_bodies_num);
                
                add_mesh(m, *m_modelScale, true);
                m_mesh = m;
                
                //m_mesh->add_child(m_spot_mesh);
            }
            catch (Exception &e)
            {
                LOG_DEBUG<<"model not found, trying in subfolders...";
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
        else if(theProperty == m_fbo_size || theProperty == m_fbo_cam_distance
                || theProperty == m_fbo_cam_fov)
        {
            gl::Fbo::Format fmt;
            int max_samples = gl::Fbo::getMaxSamples();
            
            fmt.setSamples(max_samples);
            
            m_fbo = gl::Fbo(m_fbo_size->value().x, m_fbo_size->value().y, fmt);
            
            m_free_camera->setAspectRatio(m_fbo_size->value().x / m_fbo_size->value().y);
            m_free_camera->setFov(*m_fbo_cam_fov);
            
            mat4 m = m_fbo_cam_transform->value();
            m[3].z = *m_fbo_cam_distance;
            m_fbo_cam_transform->set(m);
            
        }
        else if(theProperty == m_fbo_cam_transform || theProperty == m_fbo_cam_direction)
        {
            m_free_camera->setTransform(*m_fbo_cam_transform);
            m_free_camera->setLookAt(m_free_camera->position() + m_fbo_cam_direction->value());
            
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
        else if(theProperty == m_gravity_amount)
        {
            if(m_physics_context.dynamicsWorld())
            {
                m_physics_context.dynamicsWorld()->setGravity(btVector3(0, - *m_gravity_amount, 0));
            }
        }
        else if(theProperty == m_rigid_bodies_num || theProperty == m_rigid_bodies_size)
        {
            int num_xy = floor(sqrt(*m_rigid_bodies_num / 5.f));
            create_physics_scene(num_xy, num_xy, 5, m_material);
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
        
        else if(theProperty == m_custom_shader_paths)
        {
            if(m_custom_shader_paths->value().size() > 1)
            {
                m_custom_shader = gl::createShaderFromFile(m_custom_shader_paths->value()[0],
                                                           m_custom_shader_paths->value()[1]);
            }
        }
    }
    
    void Feldkirsche_App::tearDown()
    {
        LOG_INFO<<"ciao Feldkirsche";
    }
    
    void Feldkirsche_App::add_mesh(gl::MeshPtr the_mesh, vec3 scale, bool use_offsets)
    {
        //the_mesh = gl::Mesh::create(gl::createSphere(150, 64), gl::Material::create());
        
        if(!the_mesh) return;
        
        if(m_custom_shader)
        {
            for(auto &material : the_mesh->materials())
            {
                //if(!material->opaque())
                    material->setShader(m_custom_shader);
                material->setAmbient(gl::Color(1));
            }
        }
        
        materials().push_back(the_mesh->material());
        the_mesh->material()->setShinyness(*m_shinyness);
        the_mesh->material()->setSpecular(glm::vec4(1));
        
        if(use_offsets)
        {
            the_mesh->transform() *= glm::rotate(glm::mat4(), m_modelRotationY->value(),
                                                 vec3(0, 1, 0));
            the_mesh->setPosition(the_mesh->position() - vec3(0, the_mesh->boundingBox().min.y, 0));
            the_mesh->position() += m_modelOffset->value();
            the_mesh->setScale(scale);
        }
        
        scene().addObject(the_mesh);
        add_mesh_to_simulation(the_mesh, scale);
    }
    
    void Feldkirsche_App::add_mesh_to_simulation(gl::MeshPtr the_mesh, glm::vec3 scale)
    {
        physics::btCollisionShapePtr customShape = physics::createCollisionShape(the_mesh, scale);
        m_physics_context.collisionShapes().push_back(customShape);
        physics::MotionState *ms = new physics::MotionState(the_mesh);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(0.f, ms, customShape.get());
        btRigidBody* body = new btRigidBody(rbInfo);
        body->setFriction(0.1f);
        body->setRestitution(0.1f);
        body->setCollisionFlags( body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        body->setActivationState(DISABLE_DEACTIVATION);
        
        //add the body to the dynamics world
        m_physics_context.dynamicsWorld()->addRigidBody(body);
    }
    
    void Feldkirsche_App::create_physics_scene(int size_x, int size_y, int size_z, const gl::MaterialPtr &theMat)
    {
        scene().clear();
        m_water_objects.clear();
        
        m_physics_context.teardown_physics();
        m_physics_context.dynamicsWorld()->setGravity(btVector3(0, - *m_gravity_amount, 0));
        
        float scaling = *m_rigid_bodies_size;
        float start_pox_x = 15;
        float start_pox_y = 10;
        float start_pox_z = -3;
        
        // add static plane boundaries
        physics::btCollisionShapePtr
        ground_plane (new btStaticPlaneShape(btVector3(0, 1, 0), - m_world_half_extents->value()[1])),
        front_plane(new btStaticPlaneShape(btVector3(0, 0, -1),-m_world_half_extents->value()[2])),
        back_plane(new btStaticPlaneShape(btVector3(0, 0, 1), -m_world_half_extents->value()[2])),
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
            body->setFriction(.1f);
            body->setRestitution(0);
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
            gl::Geometry::Ptr geom = gl::createSphere(scaling, 32);//gl::createPlane(4 * scaling, 4 * scaling);
            
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
                                                                     btScalar(5 + 2.0*k + start_y),
                                                                     btScalar(2.0*j + start_z)));
                        gl::MeshPtr mesh = gl::Mesh::create(geom, theMat);
                        scene().addObject(mesh);
                        
                        float mat[16];
                        startTransform.getOpenGLMatrix(mat);
                        mesh->setTransform(glm::make_mat4(mat));
                        //mesh->setScale(vec3(random(2.f, 10.f)));
                        
                        //using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
                        physics::MotionState* myMotionState = new physics::MotionState(mesh);
                        
                        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,
                                                                        m_physics_context.collisionShapes().back().get(),
                                                                        localInertia);
                        btRigidBody* body = new btRigidBody(rbInfo);
                        body->setFriction(0.2f);
                        //body->setDamping(0.f, 2.f);
                        
                        body->setCcdMotionThreshold(10.f);
                        body->setCcdSweptSphereRadius(scaling / 4);
                        
                        m_physics_context.dynamicsWorld()->addRigidBody(body);

                        physics::physics_object obj;
                        obj.graphics_object = mesh.get();
                        obj.rigidbody = body;
                        m_water_objects.push_back(obj);
                    }   
                }
            }
        }
        LOG_DEBUG<<"created dynamicsworld with "<<
        m_physics_context.dynamicsWorld()->getNumCollisionObjects()<<" rigidbodies";
        
        // add our lights
        for (auto &light : lights()){scene().addObject(light);}
    }
}