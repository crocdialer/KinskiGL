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
