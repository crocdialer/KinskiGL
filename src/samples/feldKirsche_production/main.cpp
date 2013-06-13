#include <boost/asio/io_service.hpp>

#include "kinskiApp/ViewerApp.h"
#include "kinskiGL/Fbo.h"
#include "AssimpConnector.h"

// physics
#include "physics_context.h"

// Syphon
#include "SyphonConnector.h"

using namespace std;
using namespace kinski;
using namespace glm;

class FeldkirscheApp : public ViewerApp
{
private:
    
    std::vector<gl::Texture> m_textures;
    gl::MaterialPtr m_material;
    gl::MeshPtr m_mesh;
    gl::Font m_font;
    
    Property_<bool>::Ptr m_stepPhysics;
    Property_<string>::Ptr m_modelPath;
    Property_<glm::vec4>::Ptr m_color;
    Property_<float>::Ptr m_shinyness;
    
    // physics
    kinski::physics::physics_context m_physics_context;
    std::shared_ptr<kinski::physics::BulletDebugDrawer> m_debugDrawer;
    btRigidBody *m_ground_body, *m_left_body, *m_right_body;
    RangedProperty<int>::Ptr m_rigid_bodies_num;
    RangedProperty<float>::Ptr m_rigid_bodies_size;
    Property_<glm::vec3>::Ptr m_gravity;
    RangedProperty<float>::Ptr m_world_width;
    Property_<glm::vec3>::Ptr m_world_pos;
    
    
    // offscreen rendering
    enum DRAW_MODE{DRAW_FBO_OUTPUT = 0, DRAW_DEBUG_SCENE = 1};
    RangedProperty<int>::Ptr m_debug_draw_mode;
    gl::Scene m_debug_scene;
    gl::PerspectiveCamera::Ptr m_free_camera;
    gl::MeshPtr m_free_camera_mesh;
    gl::Fbo m_fbo;
    Property_<glm::vec2>::Ptr m_fbo_size;
    RangedProperty<float>::Ptr m_fbo_cam_distance;
    Property_<glm::mat4>::Ptr m_fbo_cam_transform;
    
    // output via Syphon
    gl::SyphonConnector m_syphon;
    Property_<bool>::Ptr m_use_syphon;
    Property_<std::string>::Ptr m_syphon_server_name;
    
public:
    
    void create_physics_scene(int size_x, int size_y, int size_z, const gl::MaterialPtr &theMat)
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
        left_plane(new btStaticPlaneShape(btVector3(1, 0, 0),- *m_world_width/2.f)),
        right_plane(new btStaticPlaneShape(btVector3(-1, 0, 0), - *m_world_width/2.f)),
        top_plane(new btStaticPlaneShape(btVector3(0, -1, 0), - *m_world_width));
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
            
            for (int k=0;k<size_y;k++)
            {
                for (int i=0;i<size_x;i++)
                {
                    for(int j = 0;j<size_z;j++)
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
        LOG_INFO<<"created dynamicsworld with "<<
        m_physics_context.dynamicsWorld()->getNumCollisionObjects()<<" rigidbodies";
    }
    
    void setup()
    {
        ViewerApp::setup();
        set_precise_selection(true);
        
        /******************** add search paths ************************/
        kinski::addSearchPath("~/Desktop");
        kinski::addSearchPath("~/Desktop/creatures", true);
        kinski::addSearchPath("~/Desktop/Feldkirsche", true);
        kinski::addSearchPath("~/Pictures");
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
        
        m_world_pos = Property_<vec3>::create("Level pos", vec3(0));
        registerProperty(m_world_pos);
        
        m_world_width = RangedProperty<float>::create("World width", 1000, 100, 5000);
        registerProperty(m_world_width);
        
        m_modelPath = Property_<string>::create("Model path", "duck.dae");
        registerProperty(m_modelPath);
        
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
        
        /********************** construct a simple scene ***********************/
        camera()->setClippingPlanes(1.0, 15000);
        
        m_free_camera = gl::PerspectiveCamera::Ptr(new gl::PerspectiveCamera(1.f, 45.f));
        
        // setup some blank textures
        m_textures.resize(1);
        
        // clear with transparent black
        gl::clearColor(gl::Color(0));
        
        m_material = gl::Material::create(gl::createShader(gl::SHADER_PHONG));
        
        // init physics pipeline
        m_physics_context.initPhysics();
        m_debugDrawer.reset(new physics::BulletDebugDrawer);
        m_physics_context.dynamicsWorld()->setDebugDrawer(m_debugDrawer.get());
        
        // load state from config file
        try
        {
            Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
        }catch(Exception &e)
        {
            LOG_WARNING << e.what();
        }
    }
    
    void update(float timeDelta)
    {
        ViewerApp::update(timeDelta);
        
        if (m_physics_context.dynamicsWorld() && *m_stepPhysics)
        {
            //m_physics_context.dynamicsWorld()->stepSimulation(timeDelta);
            auto task = boost::bind(&physics::physics_context::stepPhysics, &m_physics_context, timeDelta);
            //io_service().post(task);
            thread_pool().submit(task);
        }
        
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
    
    void draw()
    {
        // draw block
        {
            if(m_fbo){m_textures[0] = gl::render_to_texture(scene(), m_fbo, m_free_camera);}
            
            switch(*m_debug_draw_mode)
            {
                case DRAW_DEBUG_SCENE:
                    
                    gl::setMatrices(camera());
                    if(draw_grid()){gl::drawGrid(500, 500, 20, 20);}
                    //if(wireframe())
                    {
                        m_physics_context.dynamicsWorld()->debugDrawWorld();
                        m_debugDrawer->flush();
                        m_debug_scene.render(camera());
                    }
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

            if(m_mesh && h > 0)
            {

                    for (int j = 0; j < m_textures.size(); j++)
                    {
                        const gl::Texture &t = m_textures[j];
                        
                        float h = t.getHeight() * w / t.getWidth();
                        glm::vec2 step(0, h + 10);
                        drawTexture(t, vec2(w, h), offset);
                        gl::drawText2D(as_string(t.getWidth()) + std::string(" x ") +
                                       as_string(t.getHeight()), m_font, glm::vec4(1),
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
    
    
    void mousePress(const MouseEvent &e)
    {
        ViewerApp::mousePress(e);
    }
    
    void keyPress(const KeyEvent &e)
    {
        ViewerApp::keyPress(e);
        int min, max;
        
        if(!e.isShiftDown() && !e.isAltDown())
        {
            //gl::Visitor visitor;
            
            switch (e.getCode())
            {
                case KeyEvent::KEY_i:
                    //scene().root()->accept(visitor);
                    break;
                    
                case KeyEvent::KEY_d:
                    m_debug_draw_mode->getRange(min, max);
                    *m_debug_draw_mode = (*m_debug_draw_mode + 1) % (max + 1);
                    break;
                    
                case KeyEvent::KEY_p:
                    *m_stepPhysics = !*m_stepPhysics;
                    break;

                case KeyEvent::KEY_r:
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
    
    void keyRelease(const KeyEvent &e)
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
    void updateProperty(const Property::ConstPtr &theProperty)
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
        else if(theProperty == m_modelPath)
        {
            scene().removeObject(m_mesh);
            materials().clear();
            
            try
            {
                gl::MeshPtr m = gl::AssimpConnector::loadModel(*m_modelPath);
                scene().removeObject(m_mesh);
                
                // reset physics scene
                m_rigid_bodies_num->set(*m_rigid_bodies_num);
                
                add_mesh(m);
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
                        std::list<string>::const_iterator it = models.begin();
                        for (; it != models.end(); ++it)
                        {
                            add_mesh(gl::AssimpConnector::loadModel(*it));
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
    }
    
    void tearDown()
    {
        LOG_INFO<<"ciao Feldkirsche";
    }
    
    void add_mesh(const gl::MeshPtr &the_mesh)
    {
        // scale to proper size
        float scale = 7700;//(*m_world_width / 2.f) / glm::length(the_mesh->geometry()->boundingBox().halfExtents());
        
        m_mesh = the_mesh;
        materials().push_back(the_mesh->material());
        the_mesh->material()->setShinyness(*m_shinyness);
        the_mesh->material()->setSpecular(glm::vec4(1));
        the_mesh->setPosition(the_mesh->position() - vec3(0, the_mesh->boundingBox().min.y, 0));
        the_mesh->position() += m_world_pos->value();
        
        scene().addObject(m_mesh);
        
        physics::btCollisionShapePtr customShape = physics::createCollisionShape(the_mesh,
                                                                                 vec3(scale));
        m_physics_context.collisionShapes().push_back(customShape);
        physics::MotionState *ms = new physics::MotionState(m_mesh);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(0.f, ms, customShape.get());
        btRigidBody* body = new btRigidBody(rbInfo);
        body->setFriction(2.f);
        body->setCollisionFlags( body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        body->setActivationState(DISABLE_DEACTIVATION);
        
        //add the body to the dynamics world
        m_physics_context.dynamicsWorld()->addRigidBody(body);
        
        m_mesh->transform() = glm::scale(the_mesh->transform(), vec3(scale));
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new FeldkirscheApp);
    theApp->setWindowSize(1024, 1024);
    //theApp->setFullSceen();
    return theApp->run();
}
