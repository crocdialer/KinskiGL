//
//  Feldkirsche_App.cpp
//  kinskiGL
//
//  Created by Fabian on 6/17/13.
//
//

#include "Aura_App.h"
#include <boost/asio/io_service.hpp>

namespace kinski{
    
    using namespace std;
    using namespace kinski;
    using namespace glm;
    
    gl::MeshPtr Aura_App::create_fancy_cube(int num_vertices)
    {
        vec3 half_extents(50);
        
        gl::GeometryPtr geom = gl::Geometry::create();
        geom->setPrimitiveType(GL_LINE_LOOP);
        geom->vertices().resize(num_vertices);
        geom->colors().resize(num_vertices);
        
        for (auto &vertex : geom->vertices())
            vertex = glm::linearRand(-half_extents, half_extents);
        
        for (int i = 0; i < num_vertices; i++)
        {
            gl::Color color(.6f + geom->vertices()[i] / 100.f, 1.f);
            color.g = color.r - color.b;
            geom->colors()[i] = color;
        }
        geom->computeBoundingBox();
        
        gl::MaterialPtr mat = gl::Material::create();
        return gl::Mesh::create(geom, mat);
    }
    
    gl::MeshPtr Aura_App::create_fancy_lines(int num_x, int num_y)
    {
        gl::GeometryPtr geom = gl::Geometry::create();
        geom->setPrimitiveType(GL_LINE_STRIP);
        geom->vertices().resize(num_x * num_y);
        //geom->colors().resize(num_x * num_y);
        vec2 step(10);
        
        for (int i = 0; i < num_y; i++)
            for (int j = 0; j < num_x; j++)
            {
                geom->vertices()[i * num_y + j] = vec3((-num_x/2 + j) * step.x,
                                                       random(-10.f, 10.f),
                                                       (-num_y/2 + i) * step.y);
            }
        geom->computeBoundingBox();
        gl::MaterialPtr mat = gl::Material::create();
        return gl::Mesh::create(geom, mat);
    }
    
    void Aura_App::setup()
    {
        ViewerApp::setup();
        set_precise_selection(true);
        
        /******************** add search paths ************************/
        kinski::addSearchPath("~/Desktop");
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
        
        m_fbo_cam_transform = Property_<glm::mat4>::create("FBO cam transform", mat4());
        m_fbo_cam_transform->setTweakable(false);
        registerProperty(m_fbo_cam_transform);
        
        m_custom_shader_paths = Property_<std::vector<std::string> >::create("Custom shader paths",
                                                                             std::vector<std::string>());
        m_custom_shader_paths->setTweakable(false);
        registerProperty(m_custom_shader_paths);
        
        // test mesh properties
        m_num_vertices = RangedProperty<int>::create("Num vertices", 5000, 0, 200000);
        registerProperty(m_num_vertices);
        
        create_tweakbar_from_component(shared_from_this());
        observeProperties();
        
        /*********************************** init physics *****************************************/
        
        m_physics_context.initPhysics();
        m_debugDrawer.reset(new physics::BulletDebugDrawer);
        m_physics_context.dynamicsWorld()->setDebugDrawer(m_debugDrawer.get());
        
        /********************** construct a simple scene ***********************/
        camera()->setClippingPlanes(1.0, 15000);
        
        // the FBO camera
        m_free_camera = gl::PerspectiveCamera::Ptr(new gl::PerspectiveCamera(1.f, 45.f));
        
        // setup some blank textures
        m_textures.resize(2);
        
        // clear with transparent black
        gl::clearColor(gl::Color(0));
        
        m_material = gl::Material::create(gl::createShader(gl::SHADER_PHONG));
        
        m_test_sound.reset(new audio::Fmod_Sound("test.mp3"));

        // load state from config file
        try
        {
            Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
        }catch(Exception &e)
        {
            LOG_WARNING << e.what();
        }
    }
    
    void Aura_App::update(float timeDelta)
    {
        ViewerApp::update(timeDelta);
        
        // update physics
        if (m_physics_context.dynamicsWorld() && *m_stepPhysics)
        {
            //m_physics_context.dynamicsWorld()->stepSimulation(timeDelta);
            auto task = boost::bind(&physics::physics_context::stepPhysics, &m_physics_context, timeDelta);
            thread_pool().submit(task);
        }
        
        // light update
        lights().front()->setPosition(light_direction());
        
        if(m_material)
        {
            m_material->setWireframe(wireframe());
            m_material->setDiffuse(m_color->value());
            m_material->setBlending(m_color->value().a < 1.0f);
            
        }
        for (int i = 0; i < materials().size(); i++)
        {
            materials()[i]->uniform("u_time",getApplicationTime());
            materials()[i]->setShinyness(*m_shinyness);
            materials()[i]->setAmbient(0.2 * clear_color());
        }
    }
    
    void Aura_App::draw()
    {
        // draw block
        {
            gl::setMatrices(camera());
            if(draw_grid()) gl::drawGrid(500, 500);
            scene().render(camera());
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
                    if(!tex) continue;
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
    
        
    void Aura_App::mousePress(const MouseEvent &e)
    {
        ViewerApp::mousePress(e);
    }
    
    void Aura_App::keyPress(const KeyEvent &e)
    {
        ViewerApp::keyPress(e);
        int min, max;
        
        if(!e.isShiftDown() && !e.isAltDown())
        {
            switch (e.getCode())
            {
                case GLFW_KEY_W:
                    set_wireframe(!wireframe());
                    break;
                    
                case GLFW_KEY_I:
                    m_test_sound->play();
                    break;
                    
                case GLFW_KEY_D:
                    m_debug_draw_mode->getRange(min, max);
                    *m_debug_draw_mode = (*m_debug_draw_mode + 1) % (max + 1);
                    break;
                    
                case GLFW_KEY_P:
                    *m_stepPhysics = !*m_stepPhysics;
                    break;
                    
                case GLFW_KEY_R:
                    m_rigid_bodies_num->set(*m_rigid_bodies_num);
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
    
    void Aura_App::keyRelease(const KeyEvent &e)
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
    void Aura_App::updateProperty(const Property::ConstPtr &theProperty)
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
            try
            {
                LOG_WARNING<<"model loading not used";
//                gl::MeshPtr m = gl::AssimpConnector::loadModel(*m_modelPath);
//                scene().removeObject(m_mesh);
//                materials().clear();
//                m_mesh = m;
//                scene().addObject(m);
                
//                // reset physics scene
//                m_rigid_bodies_num->set(*m_rigid_bodies_num);
//                
//                add_mesh(m, *m_modelScale);
            }
            catch(Exception &e){LOG_ERROR<< e.what();}
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
        }
        else if(theProperty == m_custom_shader_paths)
        {
            if(m_custom_shader_paths->value().size() > 1)
            {
                m_custom_shader = gl::createShaderFromFile(m_custom_shader_paths->value()[0],
                                                           m_custom_shader_paths->value()[1]);
                
                if(m_custom_shader) m_material->setShader(m_custom_shader);
            }
        }
        else if (theProperty == m_num_vertices)
        {
            scene().clear();
            scene().addObject(create_fancy_cube(*m_num_vertices));
            scene().addObject(create_fancy_lines(100, 100));
        }
    }
    
    void Aura_App::tearDown()
    {
        LOG_INFO<<"ciao Aura_App";
    }
    
    void Aura_App::add_mesh(const gl::MeshPtr &the_mesh, vec3 scale)
    {
        if(!the_mesh) return;
        
        if(m_custom_shader)
        {
            for(auto &material : the_mesh->materials())
            {
                material->setShader(m_custom_shader);
            }
        }
        
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
    
    void Aura_App::create_physics_scene(int size_x, int size_y, int size_z, const gl::MaterialPtr &theMat)
    {
        
    }
}