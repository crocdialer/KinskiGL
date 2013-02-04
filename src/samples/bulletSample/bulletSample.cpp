#include "kinskiApp/TextureIO.h"

#include "kinskiGL/SerializerGL.h"
#include "kinskiGL/Scene.h"
#include "kinskiGL/Mesh.h"
#include "kinskiGL/Fbo.h"

#include "physics_context.h"

#include "kinskiCV/CVThread.h"

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


namespace kinski { namespace gl {

    class BulletDebugDrawer : public btIDebugDraw
    {
     public:
        
        BulletDebugDrawer()
        {
            gl::Material::Ptr mat(new gl::Material);
            mat->setShader(gl::createShader(gl::SHADER_UNLIT));
            gl::Geometry::Ptr geom(new gl::Geometry);
            m_mesh = gl::Mesh::Ptr(new gl::Mesh(geom, mat));
            m_mesh->geometry()->setPrimitiveType(GL_LINES);
        };
        
        virtual void drawLine(const btVector3& from,const btVector3& to,const btVector3& color)
        {
            m_mesh->geometry()->appendVertex(vec3(from.x(), from.y(), from.z()));
            m_mesh->geometry()->appendVertex(vec3(to.x(), to.y(), to.z()));
            m_mesh->geometry()->appendColor(vec4(color.x(), color.y(), color.z(), 1.0f));
            m_mesh->geometry()->appendColor(vec4(color.x(), color.y(), color.z(), 1.0f));
        }
        
        virtual void drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,
                                      btScalar distance,int lifeTime,const btVector3& color){};
        
        virtual void reportErrorWarning(const char* warningString) {LOG_WARNING<<warningString;}
        
        virtual void draw3dText(const btVector3& location,const char* textString){}
        
        virtual void setDebugMode(int debugMode){}
        
        virtual int	getDebugMode() const {return 1;}
        
        void flush()
        {
            m_mesh->geometry()->createGLBuffers();

            if(!m_mesh->vertexArray()) m_mesh->createVertexArray();
            
            gl::drawMesh(m_mesh);
            
            m_mesh->geometry()->vertices().clear();
            m_mesh->geometry()->colors().clear();
        };
        
     private:
        
        gl::Mesh::Ptr m_mesh;
    };
    

    ATTRIBUTE_ALIGNED16(struct)	MotionState : public btMotionState
    {
        gl::Object3D::Ptr m_object;
        
        btTransform m_graphicsWorldTrans;
        btTransform	m_centerOfMassOffset;
        
        BT_DECLARE_ALIGNED_ALLOCATOR();
        
        MotionState(const gl::Object3D::Ptr& theObject3D,
                    const btTransform& centerOfMassOffset = btTransform::getIdentity()):
		m_object(theObject3D),
        m_centerOfMassOffset(centerOfMassOffset)
        {
            m_graphicsWorldTrans.setFromOpenGLMatrix(&theObject3D->transform()[0][0]);
        }
        
        ///synchronizes world transform from user to physics
        virtual void getWorldTransform(btTransform& centerOfMassWorldTrans ) const
        {
			centerOfMassWorldTrans = m_centerOfMassOffset.inverse() * m_graphicsWorldTrans ;
        }
        
        ///synchronizes world transform from physics to user
        ///Bullet only calls the update of worldtransform for active objects
        virtual void setWorldTransform(const btTransform& centerOfMassWorldTrans)
        {
			m_graphicsWorldTrans = centerOfMassWorldTrans * m_centerOfMassOffset ;
            glm::mat4 transform;
            m_graphicsWorldTrans.getOpenGLMatrix(&transform[0][0]);
            m_object->setTransform(transform);
        }
        
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
    Property_<bool>::Ptr m_stepPhysics;
    Property_<glm::vec3>::Ptr m_lightDir;
    
    Property_<glm::vec4>::Ptr m_color;
    Property_<glm::mat3>::Ptr m_rotation;
    RangedProperty<float>::Ptr m_rotationSpeed;
    
    Property_<uint32_t>::Ptr m_num_visible_objects;
    
    // mouse rotation control
    vec2 m_clickPos;
    mat4 m_lastTransform, m_lastViewMatrix;
    float m_lastDistance;
    
    kinski::physics::physics_context m_physics_context;
    std::shared_ptr<kinski::gl::BulletDebugDrawer> m_debugDrawer;
    
    CVThread::Ptr m_cvThread;

public:
    
    void create_cube_stack(int size_x, int size_y, int size_z)
    {
        m_scene.objects().clear();
        
        float scaling = 8.0f;
        float start_pox_x = -5;
        float start_pox_y = -5;
        float start_pox_z = -3;
        
        ///create a few basic rigid bodies
        m_physics_context.collisionShapes().push_back(shared_ptr<btCollisionShape>(new btBoxShape(btVector3(btScalar(50.),
                                                                                        btScalar(50.),
                                                                                        btScalar(50.)))));
        gl::Mesh::Ptr groundShape(new gl::Mesh(gl::createBox(glm::vec3(50.0f)), m_material));
        m_scene.addObject(groundShape);
        groundShape->transform()[3] = glm::vec4(0, -50, 0, 1);
        
        //groundShape->initializePolyhedralFeatures();
        //	btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0,1,0),50);
        
//        btTransform groundTransform;
//        groundTransform.setIdentity();
//        groundTransform.setOrigin(btVector3(0,-50,0));
        
        {
            btScalar mass(0.);
            
            //rigidbody is dynamic if and only if mass is non zero, otherwise static
            bool isDynamic = (mass != 0.f);
            
            btVector3 localInertia(0,0,0);
            if (isDynamic)
                m_physics_context.collisionShapes().back()->calculateLocalInertia(mass,localInertia);
            
            //using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
            gl::MotionState* myMotionState = new gl::MotionState(groundShape);
            btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,
                                                            myMotionState,
                                                            m_physics_context.collisionShapes().back().get(),
                                                            localInertia);
            btRigidBody* body = new btRigidBody(rbInfo);
            //add the body to the dynamics world
            m_physics_context.dynamicsWorld()->addRigidBody(body);
        }
        
        {
            //create a few dynamic rigidbodies
            // Re-using the same collision is better for memory usage and performance
            
            m_physics_context.collisionShapes().push_back(shared_ptr<btBoxShape>(new btBoxShape(btVector3(scaling * 1,
                                                                                        scaling * 1,
                                                                                        scaling * 1))));
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
                m_physics_context.collisionShapes().back()->calculateLocalInertia(mass,localInertia);
            
            // geometry
            gl::Geometry::Ptr geom = gl::createBox(glm::vec3(scaling * 1));
            
            // material
//            gl::Material::Ptr material(new gl::Material);
//            material->setShader(gl::createShader(gl::SHADER_PHONG));
            
            float start_x = start_pox_x - size_x/2;
            float start_y = start_pox_y;
            float start_z = start_pox_z - size_z/2;
            
            for (int k=0;k<size_y;k++)
            {
                for (int i=0;i<size_x;i++)
                {
                    for(int j = 0;j<size_z;j++)
                    {
                        startTransform.setOrigin(scaling * btVector3(
                                                                   btScalar(2.0*i + start_x),
                                                                   btScalar(20+2.0*k + start_y),
                                                                   btScalar(2.0*j + start_z)));
                        
                        gl::Mesh::Ptr mesh(new gl::Mesh(geom, m_material));
                        m_scene.addObject(mesh);
                        glm::mat4 mat;
                        startTransform.getOpenGLMatrix(&mat[0][0]);
                        mesh->setTransform(mat);
                        
                        //using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
                        gl::MotionState* myMotionState = new gl::MotionState(mesh);
                        
                        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,
                                                                        m_physics_context.collisionShapes().back().get(),
                                                                        localInertia);
                        btRigidBody* body = new btRigidBody(rbInfo);
                        
                        
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
        glClearColor(1.0, 1.0, 1.0, 1.0);
        
        /*********** init our application properties ******************/
        
        m_distance = RangedProperty<float>::create("view distance", 25, 0, 5000);
        registerProperty(m_distance);
        
        m_wireFrame = Property_<bool>::create("Wireframe", false);
        registerProperty(m_wireFrame);
        
        m_drawNormals = Property_<bool>::create("Normals", false);
        registerProperty(m_drawNormals);
    
        m_stepPhysics = Property_<bool>::create("Step Physics", true);
        registerProperty(m_stepPhysics);
        
        m_lightDir = Property_<vec3>::create("Light dir", vec3(1));
        registerProperty(m_lightDir);
        
        m_color = Property_<glm::vec4>::create("Material color", glm::vec4(1 ,1 ,0, 0.6));
        registerProperty(m_color);
        
        m_rotation = Property_<glm::mat3>::create("Geometry Rotation", glm::mat3());
        registerProperty(m_rotation);
        
        m_rotationSpeed = RangedProperty<float>::create("Rotation Speed", 0, -100, 100);
        registerProperty(m_rotationSpeed);
        
        m_num_visible_objects = Property_<uint32_t>::create("Num visible objects", 0);
        
#ifndef KINSKI_RASPI
        // add properties
        addPropertyListToTweakBar(getPropertyList());
        addPropertyToTweakBar(m_num_visible_objects);
        setBarColor(vec4(0, 0 ,0 , .5));
        setBarSize(ivec2(250, 500));
#endif
        
        // enable observer mechanism
        observeProperties();
        
        /********************** construct a simple scene ***********************/
        
        gl::Fbo::Format fboFormat;
        //TODO: mulitsampling fails
        //fboFormat.setSamples(4);
        m_frameBuffer = gl::Fbo(getWidth(), getHeight(), fboFormat);

        
        m_Camera = gl::PerspectiveCamera::Ptr(new gl::PerspectiveCamera);
        m_Camera->setClippingPlanes(.1, 500);
        m_Camera->setAspectRatio(getAspectRatio());
        
        // test box shape
        gl::Geometry::Ptr myBox(gl::createBox(vec3(50, 100, 50)));
        //gl::Geometry::Ptr myBox(gl::createSphere(100, 36));
        
        m_material = gl::Material::Ptr(new gl::Material);
        m_material->setShader(gl::createShader(gl::SHADER_PHONG));
        //m_material->shader().loadFromFile("shader_normalMap.vert", "shader_normalMap.frag");
        m_material->addTexture(m_textures[0]);
//        m_material->addTexture(m_textures[1]);
//        m_textures[1].setWrapS(GL_CLAMP_TO_EDGE);
//        m_textures[1].setWrapT(GL_CLAMP_TO_EDGE);
        
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
        
        // camera input
        m_cvThread = CVThread::Ptr(new CVThread());
        m_cvThread->streamUSBCamera();
        
        // init physics pipeline
        m_physics_context.initPhysics();
        m_debugDrawer = shared_ptr<gl::BulletDebugDrawer>(new gl::BulletDebugDrawer);
        m_physics_context.dynamicsWorld()->setDebugDrawer(m_debugDrawer.get());
        
        create_cube_stack(4, 32, 4);
        
        // create a simplex noise texture
        {
            int w = 1024, h = 1024;
            float data[w * h];
            
            for (int i = 0; i < h; i++)
                for (int j = 0; j < w; j++)
                {
                    data[i * h + j] = (glm::simplex( vec3(0.0125f * vec2(i, j), 0.025)) + 1) / 2.f;
                }
            
            m_textures[1].update(data, GL_RED, w, h, true);
        }
    }
    
    void update(const float timeDelta)
    {
        *m_rotation = mat3( glm::rotate(mat4(m_rotation->val()),
                                        m_rotationSpeed->val() * timeDelta,
                                        vec3(0, 1, .5)));
        
        m_material->uniform("u_time",getApplicationTime());
        
        if (m_physics_context.dynamicsWorld() && m_stepPhysics->val())
        {
            m_physics_context.dynamicsWorld()->stepSimulation(timeDelta);
        }
        
        if(m_cvThread->hasImage())
        {
            vector<cv::Mat> images = m_cvThread->getImages();
            
            for(int i=0;i<images.size();i++)
                gl::TextureIO::updateTexture(m_textures[i], images[i]);
            
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
        m_num_visible_objects->val(m_scene.num_visible_objects());
        
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
#ifndef KINSKI_RASPI
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
#endif
    }
    
    void keyPress(const KeyEvent &e)
    {
        BaseAppType::keyPress(e);
        
        switch (e.getChar())
        {
        case KeyEvent::KEY_p:
                m_stepPhysics->val() = !m_stepPhysics->val();
            break;
                
        case KeyEvent::KEY_s:
            Serializer::saveComponentState(shared_from_this(), "config.json", PropertyIO_GL());
            break;
            
        case KeyEvent::KEY_r:
            try
            {
                Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
                m_physics_context.teardown_physics();
                create_cube_stack(4, 32, 4);
                
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
        else if(theProperty == m_wireFrame)
        {
            m_material->setWireframe(m_wireFrame->val());
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
