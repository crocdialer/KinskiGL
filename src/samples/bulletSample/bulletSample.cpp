#include "kinskiApp/TextureIO.h"

#include "kinskiGL/SerializerGL.h"
#include "kinskiGL/Scene.h"
#include "kinskiGL/Mesh.h"
#include "kinskiGL/Fbo.h"

#include "btBulletDynamicsCommon.h"

using namespace std;
using namespace kinski;
using namespace glm;

#ifdef KINSKI_RASPI
#include "kinskiApp/Raspi_App.h"
typedef Raspi_App BaseAppType;
#else
#include "kinskiApp/GLFW_App.h"
typedef GLFW_App BaseAppType;
#endif

///create 125 (5x5x5) dynamic object
#define ARRAY_SIZE_X 5
#define ARRAY_SIZE_Y 5
#define ARRAY_SIZE_Z 5

//maximum number of objects (and allow user to shoot additional boxes)
#define MAX_PROXIES (ARRAY_SIZE_X*ARRAY_SIZE_Y*ARRAY_SIZE_Z + 1024)

///scaling of the objects (0.1 = 20 centimeter boxes )
#define SCALING 8.
#define START_POS_X -5
#define START_POS_Y -5
#define START_POS_Z -3

namespace kinski { namespace gl {

    class BulletDebugDrawer : public btIDebugDraw
    {
    public:
        
        virtual void drawLine(const btVector3& from,const btVector3& to,const btVector3& color)
        {
            vector<vec3> points;
            points.push_back(vec3(from.x(), from.y(), from.z()));
            points.push_back(vec3(to.x(), to.y(), to.z()));
            
            gl::drawLines(points, vec4(color.x(), color.y(), color.z(), 1.0f));
        }
        
        virtual void drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,
                                      btScalar distance,int lifeTime,const btVector3& color){};
        
        virtual void reportErrorWarning(const char* warningString) {LOG_WARNING<<warningString;}
        
        virtual void draw3dText(const btVector3& location,const char* textString){}
        
        virtual void setDebugMode(int debugMode){}
        
        virtual int	getDebugMode() const {return 1;}
    };
}}

class BulletSample : public BaseAppType
{
private:
    
    gl::Fbo m_frameBuffer;
    gl::Texture m_textures[4];
    
    gl::Material::Ptr m_material;
    
    gl::Mesh::Ptr m_mesh;
    gl::PerspectiveCamera::Ptr m_Camera;
    gl::Scene m_scene;
    
    RangedProperty<float>::Ptr m_distance;
    Property_<bool>::Ptr m_wireFrame;
    Property_<bool>::Ptr m_drawNormals;
    Property_<glm::vec3>::Ptr m_lightDir;
    
    Property_<glm::vec4>::Ptr m_color;
    Property_<glm::mat3>::Ptr m_rotation;
    RangedProperty<float>::Ptr m_rotationSpeed;
    
    // mouse rotation control
    vec2 m_clickPos;
    mat4 m_lastTransform, m_lastViewMatrix;
    float m_lastDistance;
    
    // bullet
    shared_ptr<btCollisionShape> m_collisionShape;
    shared_ptr<btBroadphaseInterface> m_broadphase;
    shared_ptr<btCollisionDispatcher> m_dispatcher;
    shared_ptr<btConstraintSolver> m_solver;
    shared_ptr<btDefaultCollisionConfiguration> m_collisionConfiguration;
    shared_ptr<btDynamicsWorld> m_dynamicsWorld;
    
    kinski::gl::BulletDebugDrawer m_debugDrawer;

public:
    
    void initPhysics()
    {
        ///collision configuration contains default setup for memory, collision setup
        m_collisionConfiguration = shared_ptr<btDefaultCollisionConfiguration>(
            new btDefaultCollisionConfiguration());
        
        //m_collisionConfiguration->setConvexConvexMultipointIterations();
        
        ///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
        m_dispatcher = shared_ptr<btCollisionDispatcher>(new btCollisionDispatcher(m_collisionConfiguration.get()));
        
        m_broadphase = shared_ptr<btBroadphaseInterface>(new btDbvtBroadphase());
        
        ///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
        m_solver = shared_ptr<btConstraintSolver>(new btSequentialImpulseConstraintSolver);
        
        m_dynamicsWorld = shared_ptr<btDynamicsWorld>(new btDiscreteDynamicsWorld(m_dispatcher.get(),
                                                                                  m_broadphase.get(),
                                                                                  m_solver.get(),
                                                                                  m_collisionConfiguration.get()));
        m_dynamicsWorld->setDebugDrawer(&m_debugDrawer);
        
        m_dynamicsWorld->setGravity(btVector3(0,-10,0));
        
        //////////////////////////////////////////////////////////////
        
        ///create a few basic rigid bodies
        m_collisionShape = shared_ptr<btCollisionShape>(new btBoxShape(btVector3(btScalar(50.),
                                                                                 btScalar(50.),
                                                                                 btScalar(50.))));
        //groundShape->initializePolyhedralFeatures();
        //	btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0,1,0),50);
        
        btTransform groundTransform;
        groundTransform.setIdentity();
        groundTransform.setOrigin(btVector3(0,-50,0));
        
        {
            btScalar mass(0.);
            
            //rigidbody is dynamic if and only if mass is non zero, otherwise static
            bool isDynamic = (mass != 0.f);
            
            btVector3 localInertia(0,0,0);
            if (isDynamic)
                m_collisionShape->calculateLocalInertia(mass,localInertia);
            
            //using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
            btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
            btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState,
                                                            m_collisionShape.get(),localInertia);
            btRigidBody* body = new btRigidBody(rbInfo);
            
            //add the body to the dynamics world
            m_dynamicsWorld->addRigidBody(body);
        }
        
        {
            //create a few dynamic rigidbodies
            // Re-using the same collision is better for memory usage and performance
            
            btBoxShape* colShape = new btBoxShape(btVector3(SCALING*1,SCALING*1,SCALING*1));
            //btCollisionShape* colShape = new btSphereShape(btScalar(1.));
            //m_collisionShapes.push_back(colShape);
            
            /// Create Dynamic Objects
            btTransform startTransform;
            startTransform.setIdentity();
            
            btScalar	mass(1.f);
            
            //rigidbody is dynamic if and only if mass is non zero, otherwise static
            bool isDynamic = (mass != 0.f);
            
            btVector3 localInertia(0,0,0);
            if (isDynamic)
                colShape->calculateLocalInertia(mass,localInertia);
            
            float start_x = START_POS_X - ARRAY_SIZE_X/2;
            float start_y = START_POS_Y;
            float start_z = START_POS_Z - ARRAY_SIZE_Z/2;
            
            for (int k=0;k<ARRAY_SIZE_Y;k++)
            {
                for (int i=0;i<ARRAY_SIZE_X;i++)
                {
                    for(int j = 0;j<ARRAY_SIZE_Z;j++)
                    {
                        startTransform.setOrigin(SCALING*btVector3(
                                                                   btScalar(2.0*i + start_x),
                                                                   btScalar(20+2.0*k + start_y),
                                                                   btScalar(2.0*j + start_z)));
                        
                        
                        //using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
                        btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
                        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,colShape,localInertia);
                        btRigidBody* body = new btRigidBody(rbInfo);
                        
                        
                        m_dynamicsWorld->addRigidBody(body);
                    }
                }
            }
        }
    }
    
    void setup()
    {
        /*********** init our application properties ******************/
        
        m_distance = RangedProperty<float>::create("view distance", 25, 0, 5000);
        registerProperty(m_distance);
        
        m_wireFrame = Property_<bool>::create("Wireframe", false);
        registerProperty(m_wireFrame);
        
        m_drawNormals = Property_<bool>::create("Normals", false);
        registerProperty(m_drawNormals);
        
        m_lightDir = Property_<vec3>::create("Light dir", vec3(1));
        registerProperty(m_lightDir);
        
        m_color = Property_<glm::vec4>::create("Material color", glm::vec4(1 ,1 ,0, 0.6));
        registerProperty(m_color);
        
        m_rotation = Property_<glm::mat3>::create("Geometry Rotation", glm::mat3());
        registerProperty(m_rotation);
        
        m_rotationSpeed = RangedProperty<float>::create("Rotation Speed", 0, -100, 100);
        registerProperty(m_rotationSpeed);
        
        // add properties
        addPropertyListToTweakBar(getPropertyList());
        
        setBarColor(vec4(0, 0 ,0 , .5));
        setBarSize(ivec2(250, 500));

        // enable observer mechanism
        observeProperties();
        
        /********************** construct a simple scene ***********************/
        
        gl::Fbo::Format fboFormat;
        //TODO: mulitsampling fails
        //fboFormat.setSamples(4);
        m_frameBuffer = gl::Fbo(getWidth(), getHeight(), fboFormat);

        
        m_Camera = gl::PerspectiveCamera::Ptr(new gl::PerspectiveCamera);
        m_Camera->setClippingPlanes(.1, 5000);
        
        // test box shape
        gl::Geometry::Ptr myBox(gl::createBox(vec3(50, 100, 50)));
        //gl::Geometry::Ptr myBox(gl::createSphere(100, 36));
        
        m_material = gl::Material::Ptr(new gl::Material);
        m_material->setShader(gl::createShader(gl::SHADER_PHONG));
        
        gl::Mesh::Ptr myBoxMesh(new gl::Mesh(myBox, m_material));
        myBoxMesh->setPosition(vec3(0, -100, 0));
        //m_scene.addObject(myBoxMesh);
        
        // load state from config file
        try
        {
            Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
        }catch(Exception &e)
        {
            LOG_WARNING << e.what();
        }
        
        // init physics pipeline
        initPhysics();
    }
    
    void update(const float timeDelta)
    {
        *m_rotation = mat3( glm::rotate(mat4(m_rotation->val()),
                                        m_rotationSpeed->val() * timeDelta,
                                        vec3(0, 1, .5)));
        
        m_material->uniform("u_time",getApplicationTime());
        
        if (m_dynamicsWorld)
        {
            m_dynamicsWorld->stepSimulation(timeDelta);
        }
    }
    
    void draw()
    {
//        m_frameBuffer.bindFramebuffer();
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//        glViewport(0, 0, m_frameBuffer.getWidth(), m_frameBuffer.getHeight());

        gl::loadMatrix(gl::PROJECTION_MATRIX, m_Camera->getProjectionMatrix());
        gl::loadMatrix(gl::MODEL_VIEW_MATRIX, m_Camera->getViewMatrix());
        gl::drawGrid(500, 500);
        
        m_scene.render(m_Camera);
        
        if (m_dynamicsWorld)
            m_dynamicsWorld->debugDrawWorld();
        
//        m_frameBuffer.unbindFramebuffer();
//        glViewport(0, 0, getWidth(), getHeight());
//        gl::drawTexture(m_frameBuffer.getTexture(), windowSize() );
    }
    
    void mousePress(const MouseEvent &e)
    {
        m_clickPos = vec2(e.getX(), e.getY());
        m_lastTransform = mat4(m_rotation->val());
        m_lastViewMatrix = m_Camera->getViewMatrix();
        m_lastDistance = m_distance->val();
    }
    
    void mouseDrag(const MouseEvent &e)
    {
        vec2 mouseDiff = vec2(e.getX(), e.getY()) - m_clickPos;
        if(e.isLeft() && (e.isAltDown() || !displayTweakBar()))
        {
            mat4 mouseRotate = glm::rotate(m_lastTransform, mouseDiff.y, vec3(m_lastViewMatrix[0]) );
            mouseRotate = glm::rotate(mouseRotate, mouseDiff.x, vec3(0 , 1, 0) );
            
            *m_rotation = mat3(mouseRotate);
        }
        else if(e.isRight())
        {
            *m_distance = m_lastDistance + 0.3f * mouseDiff.y;
        }
    }
    
    void keyPress(const KeyEvent &e)
    {
        GLFW_App::keyPress(e);
        
        switch (e.getChar())
        {
        case KeyEvent::KEY_s:
            Serializer::saveComponentState(shared_from_this(), "config.json", PropertyIO_GL());
            break;
            
        case KeyEvent::KEY_r:
            try
            {
                Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
            }catch(Exception &e)
            {
                LOG_WARNING << e.what();
            }
            break;
                
        default:
            break;
        }
    }

    void resize(int w, int h)
    {
        m_Camera->setAspectRatio(getAspectRatio());
        gl::Fbo::Format fboFormat;
        //TODO: mulitsampling fails
        //fboFormat.setSamples(4);
        m_frameBuffer = gl::Fbo(w, h, fboFormat);
    }
    
    // Property observer callback
    void updateProperty(const Property::ConstPtr &theProperty)
    {
        // one of our porperties was changed
        if(theProperty == m_color)
        {
            m_material->setDiffuse(m_color->val());
        }
        else if(theProperty == m_lightDir)
        {
            m_material->uniform("u_lightDir", m_lightDir->val());
        }
        else if(theProperty == m_distance ||
                theProperty == m_rotation)
        {
            m_Camera->setPosition( m_rotation->val() * glm::vec3(0, 0, m_distance->val()) );
            m_Camera->setLookAt(glm::vec3(0, 0, 0));
        }
    }
    
    void tearDown()
    {
        LOG_PRINT<<"ciao bullet sample";
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new BulletSample);
    theApp->setWindowSize(1024, 600);
    
    return theApp->run();
}
