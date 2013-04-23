//
//  physics_context.h
//  kinskiGL
//
//  Created by Fabian on 1/16/13.
//
//

#ifndef __kinskiGL__physics_context__
#define __kinskiGL__physics_context__

#include <boost/function.hpp>
#include "kinskiCore/Definitions.h"
#include "btBulletDynamicsCommon.h"

#include "kinskiGL/Mesh.h"

namespace kinski { namespace gl {
    
    class BulletDebugDrawer : public btIDebugDraw
    {
    public:
        
        BulletDebugDrawer()
        {
            gl::MaterialPtr mat(new gl::Material);
            mat->setShader(gl::createShader(gl::SHADER_UNLIT));
            gl::GeometryPtr geom(new gl::Geometry);
            m_mesh = gl::Mesh::create(geom, mat);
            m_mesh->geometry()->setPrimitiveType(GL_LINES);
        };
        
        virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
        {
            m_mesh->geometry()->appendVertex(glm::vec3(from.x(), from.y(), from.z()));
            m_mesh->geometry()->appendVertex(glm::vec3(to.x(), to.y(), to.z()));
            m_mesh->geometry()->appendColor(glm::vec4(color.x(), color.y(), color.z(), 1.0f));
            m_mesh->geometry()->appendColor(glm::vec4(color.x(), color.y(), color.z(), 1.0f));
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

namespace kinski{ namespace physics{

    class physics_context
    {
     public:
        
        void initPhysics();
        void teardown_physics();
        
        const std::shared_ptr<const btDynamicsWorld> dynamicsWorld() const {return m_dynamicsWorld;};
        const std::shared_ptr<btDynamicsWorld>& dynamicsWorld() {return m_dynamicsWorld;};
        
        const std::vector<std::shared_ptr<btCollisionShape> >& collisionShapes() const {return m_collisionShapes;};
        std::vector<std::shared_ptr<btCollisionShape> >& collisionShapes() {return m_collisionShapes;};
        
     private:
        
        std::vector<std::shared_ptr<btCollisionShape> > m_collisionShapes;
        std::shared_ptr<btBroadphaseInterface> m_broadphase;
        std::shared_ptr<btCollisionDispatcher> m_dispatcher;
        std::shared_ptr<btConstraintSolver> m_solver;
        std::shared_ptr<btDefaultCollisionConfiguration> m_collisionConfiguration;
        std::shared_ptr<btDynamicsWorld> m_dynamicsWorld;
        
//        boost::function<void (btBroadphasePair& collisionPair, btCollisionDispatcher& dispatcher,
//            btDispatcherInfo& dispatchInfo)> m_nearCallback;
        
    };
}}//namespace

#endif /* defined(__kinskiGL__physics_context__) */
