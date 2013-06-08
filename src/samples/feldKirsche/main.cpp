#include "kinskiApp/ViewerApp.h"
#include "kinskiCV/CVThread.h"
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
    
    kinski::physics::physics_context m_physics_context;
    std::shared_ptr<kinski::physics::BulletDebugDrawer> m_debugDrawer;
    
    btRigidBody *m_ground_body, *m_left_body, *m_right_body;
    
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
        
        m_modelPath = Property_<string>::create("Model path", "duck.dae");
        registerProperty(m_modelPath);
        
        m_color = Property_<glm::vec4>::create("Material color", glm::vec4(1 ,1 ,0, 0.6));
        registerProperty(m_color);
        
        m_shinyness = Property_<float>::create("Shinyness", 1.0);
        registerProperty(m_shinyness);
        
        create_tweakbar_from_component(shared_from_this());

        // enable observer mechanism
        observeProperties();
        
        /********************** construct a simple scene ***********************/
        camera()->setClippingPlanes(1.0, 15000);
        
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
        ViewerApp::keyPress(e);
        
        switch (e.getCode())
        {
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
    }
    
    void keyRelease(const KeyEvent &e)
    {
        ViewerApp::keyRelease(e);
        
        switch (e.getCode())
        {
            case GLFW_KEY_UP:
                m_ground_body->getWorldTransform().setOrigin(btVector3(0, 0, 0));
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
