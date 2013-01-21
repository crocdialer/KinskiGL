//
//  physics_context.cpp
//  kinskiGL
//
//  Created by Fabian on 1/16/13.
//
//

#include "physics_context.h"

#include "kinskiCore/Exception.h"
#include "kinskiCore/Logger.h"
#include "kinskiCore/file_functions.h"

using namespace std;

namespace kinski{ namespace physics{
    
    void physics_context::initPhysics()
    {
        LOG_INFO<<"initializing physics";
        
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
        
        m_dynamicsWorld->setGravity(btVector3(0,-500,0));
        
        //////////////////////////////////////////////////////////////
        
        //m_dispatcher->setNearCallback(m_nearCallback.);
    }
    
    void physics_context::teardown_physics()
    {
        int i;
        for (i=m_dynamicsWorld->getNumCollisionObjects()-1; i>=0 ;i--)
        {
            btCollisionObject* obj = m_dynamicsWorld->getCollisionObjectArray()[i];
            btRigidBody* body = btRigidBody::upcast(obj);
            if (body && body->getMotionState())
            {
                delete body->getMotionState();
            }
            m_dynamicsWorld->removeCollisionObject( obj );
            delete obj;
        }
        
        m_collisionShapes.clear();
    }
    
//    void MyNearCallback(btBroadphasePair& collisionPair, btCollisionDispatcher& dispatcher,
//                        btDispatcherInfo& dispatchInfo)
//    {
//        // Do your collision logic here
//        // Only dispatch the Bullet collision information if you want the physics to continue
//        dispatcher.defaultNearCallback(collisionPair, dispatcher, dispatchInfo);
//    }
}}
