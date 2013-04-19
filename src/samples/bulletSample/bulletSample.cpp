#ifdef KINSKI_RASPI
#include "kinskiApp/Raspi_App.h"
typedef kinski::Raspi_App BaseAppType;
#else
#include "kinskiApp/ViewerApp.h"
typedef kinski::ViewerApp BaseAppType;
#endif

#include "kinskiApp/TextureIO.h"
#include "physics_context.h"
#include "kinskiCV/CVThread.h"
#include "ThreshNode.h"
#include "DopeRecorder.h"
#include "FaceFilter.h"

using namespace std;
using namespace kinski;
using namespace glm;

namespace kinski { namespace gl {

    class BulletDebugDrawer : public btIDebugDraw
    {
     public:
        
        BulletDebugDrawer()
        {
            gl::MaterialPtr mat(new gl::Material);
            mat->setShader(gl::createShader(gl::SHADER_UNLIT));
            gl::GeometryPtr geom(new gl::Geometry);
            m_mesh = gl::MeshPtr(new gl::Mesh(geom, mat));
            m_mesh->geometry()->setPrimitiveType(GL_LINES);
        };
        
        virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
        {
            m_mesh->geometry()->appendVertex(vec3(from.x(), from.y(), from.z()));
            m_mesh->geometry()->appendVertex(vec3(to.x(), to.y(), to.z()));
            m_mesh->geometry()->appendColor(vec4(color.x(), color.y(), color.z(), 1.0f));
            m_mesh->geometry()->appendColor(vec4(color.x(), color.y(), color.z(), 1.0f));
        }
        
        virtual void drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,
                                      btScalar distance,int lifeTime,const btVector3& color){};
        
        virtual void reportErrorWarning(const char* warningString) {LOG_WARNING<<warningString;}
        virtual void draw3dText(const btVector3& location,const char* textString)
        {
            //TODO: font rendering here
        }
        virtual void setDebugMode(int debugMode){LOG_WARNING<<"unsupported operation";}
        virtual int	getDebugMode() const {return DBG_DrawWireframe;}
        
        void flush()
        {
            m_mesh->geometry()->createGLBuffers();
            gl::drawMesh(m_mesh);
            m_mesh->geometry()->vertices().clear();
            m_mesh->geometry()->colors().clear();
            m_mesh->geometry()->vertices().reserve(1024);
            m_mesh->geometry()->colors().reserve(1024);
        };
        
     private:
        gl::Mesh::Ptr m_mesh;
    };
    

    ATTRIBUTE_ALIGNED16(struct)	MotionState : public btMotionState
    {
        gl::Object3DPtr m_object;
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
    
    vector<gl::Texture> m_textures;
    //gl::Texture m_textures[4];
    Property_<string>::Ptr m_font_name;
    Property_<int>::Ptr m_font_size;
    Property_<bool>::Ptr m_stepPhysics;
    Property_<glm::vec4>::Ptr m_color;
    Property_<uint32_t>::Ptr m_num_visible_objects;
    kinski::physics::physics_context m_physics_context;
    std::shared_ptr<kinski::gl::BulletDebugDrawer> m_debugDrawer;
    
    gl::Font m_font;
    std::list<std::string> m_font_paths;
    gl::MeshPtr m_label;
    
    // opencv interface
    CVThread::Ptr m_cvThread;

public:
    
    void create_cube_stack(int size_x, int size_y, int size_z)
    {
        scene().objects().clear();
        m_physics_context.collisionShapes().clear();
        
        float scaling = 8.0f;
        float start_pox_x = -5;
        float start_pox_y = -5;
        float start_pox_z = -3;
        
        ///create a few basic rigid bodies
        m_physics_context.collisionShapes().push_back(
            shared_ptr<btCollisionShape>(new btBoxShape(btVector3(btScalar(50.),
                                                                  btScalar(50.),
                                                                  btScalar(50.)))));
        gl::MeshPtr groundShape(new gl::Mesh(gl::createBox(glm::vec3(50.0f)), materials()[0]));
        scene().addObject(groundShape);
        groundShape->transform()[3] = glm::vec4(0, -50, 0, 1);
        
        //groundShape->initializePolyhedralFeatures();
        shared_ptr<btCollisionShape> plane_shape (new btStaticPlaneShape(btVector3(0,1,0),50));
        
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
                        
                        gl::Mesh::Ptr mesh(new gl::Mesh(geom, materials()[0]));
                        scene().addObject(mesh);
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
        BaseAppType::setup();
        
        kinski::addSearchPath("~/Desktop");
        kinski::addSearchPath("/Library/Fonts");
        m_font_paths = kinski::getDirectoryEntries(getSearchPaths().back(), false, "ttf");
        m_font.load("Arial.ttf", 24);
        for (auto i = 0; i < 4; ++i){ m_textures.push_back(gl::Texture()); }
        
        /*********** init our application properties ******************/
        
        m_font_name = Property_<string>::create("Font Name", "Arial.ttf");
        registerProperty(m_font_name);
        
        m_font_size = Property_<int>::create("Font Size", 24);
        registerProperty(m_font_size);
        
        m_stepPhysics = Property_<bool>::create("Step Physics", true);
        registerProperty(m_stepPhysics);
        
        m_color = Property_<glm::vec4>::create("Material color", glm::vec4(1 ,1 ,0, 0.6));
        registerProperty(m_color);
        
        m_num_visible_objects = Property_<uint32_t>::create("Num visible objects", 0);
            
        // enable observer mechanism
        observeProperties();
        
        /********************** construct a simple scene ***********************/
        
        // test box shape
        gl::Geometry::Ptr myBox(gl::createBox(vec3(50, 100, 50)));
        
        materials()[0]->setShader(gl::createShader(gl::SHADER_PHONG));
        //materials()[0]->setShader(gl::createShaderFromFile("shader_normalMap.vert", "shader_normalMap.frag"));
        
        materials()[0]->addTexture(m_textures[3]);
        //materials()[0]->addTexture(m_textures[3]);

        // camera input
        m_cvThread = CVThread::create();
        CVProcessNode::Ptr thresh_node(new ThreshNode(-1)), record_node(new DopeRecorder(5000)),
            face_node(new FaceFilter());
        CVCombinedProcessNode::Ptr combi_node = face_node >> thresh_node >> record_node;
        combi_node->observeProperties();
        m_cvThread->setProcessingNode(combi_node);
        m_cvThread->streamUSBCamera();
        
        // init physics pipeline
        m_physics_context.initPhysics();
        m_debugDrawer = shared_ptr<gl::BulletDebugDrawer>(new gl::BulletDebugDrawer);
        m_physics_context.dynamicsWorld()->setDebugDrawer(m_debugDrawer.get());
        
        // create a physics scene
        create_cube_stack(4, 32, 4);
        
        // create a simplex noise texture
        if(false)
        {
            int w = 512, h = 512;
            float data[w * h];
            
            for (int i = 0; i < h; i++)
                for (int j = 0; j < w; j++)
                {
                    data[i * h + j] = (glm::simplex( vec3(0.0125f * vec2(i, j), 0.025)) + 1) / 2.f;
                }
            
            gl::Texture::Format fmt;
            fmt.setInternalFormat(GL_RED);
            fmt.set_mipmapping(true);
            m_textures[1] = gl::Texture (w, h, fmt);
            m_textures[1].update(data, GL_RED, w, h, true);
        }
        
        // load state from config file
        try
        {
            Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
            Serializer::loadComponentState(m_cvThread->getProcessingNode(), "config_cv.json", PropertyIO_GL());
        }catch(Exception &e)
        {
            LOG_WARNING << e.what();
        }
#ifndef KINSKI_RASPI
        create_tweakbar_from_component(shared_from_this());
        create_tweakbar_from_component(m_cvThread);
        addPropertyListToTweakBar(m_cvThread->getProcessingNode()->getPropertyList(),
                                  "", tweakBars().back());
#endif
    }
    
    void update(const float timeDelta)
    {
        BaseAppType::update(timeDelta);
        
        if (m_physics_context.dynamicsWorld() && *m_stepPhysics)
        {
            m_physics_context.dynamicsWorld()->stepSimulation(timeDelta);
        }
        
        if(m_cvThread->hasImage())
        {
            vector<cv::Mat> images = m_cvThread->getImages();
            
            for(int i = 0;i < images.size();i++)
            {
                if(i < 4)
                {
                    gl::TextureIO::updateTexture(m_textures[i], images[i]);
                }
            }
        }
        for (int i = 0; i < materials().size(); i++)
        {
            materials()[i]->uniform("u_time",getApplicationTime());
            materials()[i]->uniform("u_lightDir", light_direction());
            materials()[i]->setAmbient(0.2 * clear_color());
        }
    }
    
    void draw()
    {
        gl::setMatrices(camera());
        
        if(draw_grid())
        {
            gl::drawGrid(500, 500);
        }
        
        if(wireframe())
        {
            m_physics_context.dynamicsWorld()->debugDrawWorld();
            m_debugDrawer->flush();
        }else
        {
            scene().render(camera());
            *m_num_visible_objects = scene().num_visible_objects();
        }
        
        if(selected_mesh())
        {
            gl::loadMatrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix() * selected_mesh()->transform());
            gl::drawAxes(selected_mesh());
            gl::drawBoundingBox(selected_mesh());
            if(normals()) gl::drawNormals(selected_mesh());
            
            gl::loadMatrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix() * m_label->transform());
            m_label->setRotation(glm::mat3(camera()->transform()));
            gl::drawMesh(m_label);
        }
        // draw texture map(s)
        if(displayTweakBar())
        {
            // draw opencv maps
            glm::vec2 offset(getWidth() - getWidth()/6.f - 10, getHeight() - 10);
            glm::vec2 step(0, - getHeight()/6.f - 10);
            for(int i = 0;i<4;i++)
            {
                drawTexture(m_textures[i], windowSize()/6.f, offset);
//                gl::drawText2D(as_string(m_textures[i].getWidth()) + std::string(" x ") +
//                                    as_string(m_textures[i].getHeight()),
//                               m_font, offset);
                offset += step;
            }
            // draw fps string
            gl::drawText2D(kinski::as_string(framesPerSec()), m_font,
                           glm::vec2(windowSize().x - 110, 10));
        }
    }
    
    void keyPress(const KeyEvent &e)
    {
        BaseAppType::keyPress(e);
        
        switch (e.getChar())
        {
        case KeyEvent::KEY_p:
            *m_stepPhysics = !*m_stepPhysics;
            break;
                
        case KeyEvent::KEY_r:
            Serializer::loadComponentState(m_cvThread->getProcessingNode(), "config_cv.json", PropertyIO_GL());
            m_physics_context.teardown_physics();
            create_cube_stack(4, 32, 4);
            break;
                
        case KeyEvent::KEY_s:
            try
            {
                Serializer::saveComponentState(m_cvThread->getProcessingNode(), "config_cv.json", PropertyIO_GL());
            }catch(const Exception &e){ LOG_ERROR<<e.what(); }
            break;
                
        default:
            break;
        }
    }
    
    void mousePress(const MouseEvent &e)
    {
        BaseAppType::mousePress(e);
        
        if(selected_mesh())
        {
            m_label = m_font.create_mesh("My Id is " + kinski::as_string(selected_mesh()->getID()));
            m_label->setPosition(selected_mesh()->position()
                                 + camera()->up() * (selected_mesh()->boundingBox().height() / 2.f
                                                     + m_label->boundingBox().height())
                                 - m_label->boundingBox().center());
            m_label->setRotation(glm::mat3(camera()->transform()));
        }
        
    }
    
    // Property observer callback
    void updateProperty(const Property::ConstPtr &theProperty)
    {
        BaseAppType::updateProperty(theProperty);
        
        if(theProperty == m_color)
        {
            materials()[0]->setDiffuse(*m_color);
        }
        else if(theProperty == m_font_name || theProperty == m_font_size)
        {
            m_font.load(*m_font_name, *m_font_size);
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
