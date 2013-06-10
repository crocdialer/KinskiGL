#include "kinskiApp/ViewerApp.h"
#include "kinskiGL/Fbo.h"
#include "AssimpConnector.h"
#include "physics_context.h"

using namespace std;
using namespace kinski;
using namespace glm;

class FeldkirscheApp : public ViewerApp
{
private:
    
    gl::Texture m_textures[4];
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
    Property_<glm::vec3>::Ptr m_gravity;
    
    // offscreen rendering
    enum DRAW_MODE{DRAW_NOTHING = 0, DRAW_FBO_OUTPUT = 1, DRAW_DEBUG_SCENE = 2};
    RangedProperty<int>::Ptr m_debug_draw_mode;
    gl::Scene m_debug_scene;
    gl::PerspectiveCamera::Ptr m_free_camera;
    gl::MeshPtr m_free_camera_mesh;
    gl::Fbo m_fbo;
    Property_<glm::vec2>::Ptr m_fbo_size;
    RangedProperty<float>::Ptr m_fbo_cam_distance;
    
public:
    
    void create_physics_scene(int size_x, int size_y, int size_z, const gl::MaterialPtr &theMat)
    {
        scene().objects().clear();
        m_physics_context.collisionShapes().clear();
        m_physics_context.dynamicsWorld()->setGravity(btVector3(0, -981.f, 0));
        
        float scaling = 20.0f;
        float start_pox_x = -5;
        float start_pox_y = -5;
        float start_pox_z = -3;
        
        // add static plane boundaries
        physics::btCollisionShapePtr ground_plane (new btStaticPlaneShape(btVector3(0, 1, 0), 0)),
        front_plane(new btStaticPlaneShape(btVector3(0, 0, -1),-150)),
        back_plane(new btStaticPlaneShape(btVector3(0, 0, 1), -150)),
        left_plane(new btStaticPlaneShape(btVector3(1, 0, 0),-2000)),
        right_plane(new btStaticPlaneShape(btVector3(-1, 0, 0), -2000));
        m_physics_context.collisionShapes().push_back(ground_plane);
        m_physics_context.collisionShapes().push_back(front_plane);
        m_physics_context.collisionShapes().push_back(back_plane);
        m_physics_context.collisionShapes().push_back(left_plane);
        m_physics_context.collisionShapes().push_back(right_plane);

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
                        body->setCcdMotionThreshold(scaling / 4);
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
        kinski::addSearchPath("~/Pictures");
        kinski::addSearchPath("/Library/Fonts");
        m_font.load("Courier New Bold.ttf", 24);
        
        /*********** init our application properties ******************/
        
        m_stepPhysics = Property_<bool>::create("Step physics", true);
        registerProperty(m_stepPhysics);
        
        m_gravity = Property_<vec3>::create("Gravity", vec3(0, -1, 0));
        registerProperty(m_gravity);
        
        m_modelPath = Property_<string>::create("Model path", "duck.dae");
        registerProperty(m_modelPath);
        
        m_color = Property_<glm::vec4>::create("Material color", glm::vec4(1 ,1 ,0, 0.6));
        registerProperty(m_color);
        
        m_shinyness = Property_<float>::create("Shinyness", 1.0);
        registerProperty(m_shinyness);
        
        m_debug_draw_mode = RangedProperty<int>::create("Debug draw mode", 0, 0, 2);
        registerProperty(m_debug_draw_mode);
        m_fbo_size = Property_<glm::vec2>::create("Fbo size", vec2(1024));
        registerProperty(m_fbo_size);
        m_fbo_cam_distance = RangedProperty<float>::create("Fbo cam distance", 200.f, 0.f, 10000.f);
        registerProperty(m_fbo_cam_distance);
        
        create_tweakbar_from_component(shared_from_this());
        observeProperties();
        
        /********************** construct a simple scene ***********************/
        camera()->setClippingPlanes(1.0, 15000);
        
        m_free_camera = gl::PerspectiveCamera::Ptr(new gl::PerspectiveCamera(1.f, 45.f));
        
        // clear with transparent black
        gl::clearColor(gl::Color(0));
        
        m_material = gl::Material::create(gl::createShader(gl::SHADER_PHONG));
        
        // init physics pipeline
        m_physics_context.initPhysics();
        m_debugDrawer = shared_ptr<physics::BulletDebugDrawer>(new physics::BulletDebugDrawer);
        m_physics_context.dynamicsWorld()->setDebugDrawer(m_debugDrawer.get());
        
        create_physics_scene(25, 50, 1, m_material);
        
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
            m_physics_context.dynamicsWorld()->stepSimulation(timeDelta);
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
            //background
            //gl::drawTexture(m_textures[0], windowSize());
            gl::setMatrices(camera());
            
            if(draw_grid()){ gl::drawGrid(500, 500, 20, 20); }
            
            if(wireframe())
            {
                m_physics_context.dynamicsWorld()->debugDrawWorld();
                m_debugDrawer->flush();
            }
            else
            {
                scene().render(camera());
            }
            
        }// FBO block
        
        // draw texture map(s)
        if(displayTweakBar())
        {
            float w = (windowSize()/8.f).x;
            float h = m_textures[0].getHeight() * w / m_textures[0].getWidth();
            glm::vec2 offset(getWidth() - w - 10, 10);
            glm::vec2 step(0, h + 10);
            
            if(m_mesh && h > 0)
            {
                for(int i = 0;i < m_mesh->materials().size();i++)
                {
                    gl::MaterialPtr m = m_mesh->materials()[i];
                    
                    for (int j = 0; j < m->textures().size(); j++)
                    {
                        const gl::Texture &t = m->textures()[j];
                        
                        float h = t.getHeight() * w / t.getWidth();
                        glm::vec2 step(0, h + 10);
                        drawTexture(t, vec2(w, h), offset);
                        gl::drawText2D(as_string(t.getWidth()) + std::string(" x ") +
                                       as_string(t.getHeight()), m_font, glm::vec4(1),
                                       offset);
                        offset += step;
                    }
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
        int min, max;
        
        switch (e.getCode())
        {
            case KeyEvent::KEY_d:
                m_debug_draw_mode->getRange(min, max);
                *m_debug_draw_mode = (*m_debug_draw_mode + 1) % (max + 1);
                break;
                
            case KeyEvent::KEY_p:
                *m_stepPhysics = !*m_stepPhysics;
                break;
                
            case KeyEvent::KEY_f:
                setFullSceen(!fullSceen());
                break;

            case KeyEvent::KEY_r:
                m_physics_context.teardown_physics();
                create_physics_scene(25, 50, 1, m_material);
                break;
                
            case GLFW_KEY_UP:
                LOG_DEBUG<<"TILT UP";
                m_ground_body->getWorldTransform().setOrigin(btVector3(0, 50, 0));
                break;
            
            case GLFW_KEY_LEFT:
                LOG_DEBUG<<"TILT LEFT";
                m_left_body->getWorldTransform().setOrigin(btVector3(200, 0, 0));
                break;
            
            case GLFW_KEY_RIGHT:
                LOG_DEBUG<<"TILT RIGHT";
                m_right_body->getWorldTransform().setOrigin(btVector3(-200, 0, 0));
                break;
                
            default:
                break;
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
        ViewerApp::keyPress(e);
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
            try
            {
                gl::MeshPtr m = gl::AssimpConnector::loadModel(*m_modelPath);
                scene().removeObject(m_mesh);
                m_mesh = m;
                materials().clear();
                materials().push_back(m->material());
                m->material()->setShinyness(*m_shinyness);
                m->material()->setSpecular(glm::vec4(1));
                m->setPosition(m->position() - vec3(0, m->boundingBox().min.y, 0));
                scene().addObject(m_mesh);
                
                physics::btCollisionShapePtr customShape = physics::createCollisionShape(m->geometry());
                m_physics_context.collisionShapes().push_back(customShape);
                physics::MotionState *ms = new physics::MotionState(m_mesh);
                btRigidBody::btRigidBodyConstructionInfo rbInfo(0.f, ms, customShape.get());
                btRigidBody* body = new btRigidBody(rbInfo);
                body->setFriction(2.f);
                body->setCollisionFlags( body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
                body->setActivationState(DISABLE_DEACTIVATION);
                
                //add the body to the dynamics world
                m_physics_context.dynamicsWorld()->addRigidBody(body);
            }
            catch (Exception &e){ LOG_ERROR<< e.what(); }
        }
        else if(theProperty == m_fbo_size || theProperty == m_fbo_cam_distance)
        {
            m_fbo = gl::Fbo(m_fbo_size->value().x, m_fbo_size->value().y);
            m_free_camera->setAspectRatio(m_fbo_size->value().x / m_fbo_size->value().y);
            if(m_mesh)
                m_free_camera->setPosition(m_mesh->position() + vec3(0, 0, *m_fbo_cam_distance));
            
            m_debug_scene.removeObject(m_free_camera_mesh);
            m_free_camera_mesh = gl::createFrustumMesh(m_free_camera);
            m_debug_scene.addObject(m_free_camera_mesh);
        }
        else if(theProperty == m_gravity)
        {
            vec3 gravity_vec = glm::normalize(m_gravity->value()) * m_physics_context.dynamicsWorld()->getGravity().length();
            m_physics_context.dynamicsWorld()->setGravity(physics::type_cast(gravity_vec));
        }
    }
    
    void tearDown()
    {
        LOG_PRINT<<"ciao Feldkirsche";
    }
    
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new FeldkirscheApp);
    theApp->setWindowSize(1024, 768);
    //theApp->setFullSceen();
    return theApp->run();
}
