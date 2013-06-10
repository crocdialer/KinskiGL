//
//  physics_context.cpp
//  kinskiGL
//
//  Created by Fabian on 1/16/13.
//
//

#include <boost/bind.hpp>
#include "physics_context.h"
#include "kinskiCore/Exception.h"
#include "kinskiCore/Logger.h"
#include "kinskiCore/file_functions.h"

#define USE_PARALLEL_SOLVER 1 //experimental parallel solver
#define USE_PARALLEL_DISPATCHER 1

#include "btBulletDynamicsCommon.h"
#include "BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btSimulationIslandManager.h"

#ifdef USE_PARALLEL_DISPATCHER
#include "BulletMultiThreaded/SpuGatheringCollisionDispatcher.h"
#include "BulletMultiThreaded/PlatformDefinitions.h"

#ifdef USE_LIBSPE2
#include "BulletMultiThreaded/SpuLibspe2Support.h"
#elif defined (_WIN32)
#include "BulletMultiThreaded/Win32ThreadSupport.h"
#include "BulletMultiThreaded/SpuNarrowPhaseCollisionTask/SpuGatheringCollisionTask.h"

#elif defined (USE_PTHREADS)

#include "BulletMultiThreaded/PosixThreadSupport.h"
#include "BulletMultiThreaded/SpuNarrowPhaseCollisionTask/SpuGatheringCollisionTask.h"

#else
//other platforms run the parallel code sequentially (until pthread support or other parallel implementation is added)

#include "BulletMultiThreaded/SpuNarrowPhaseCollisionTask/SpuGatheringCollisionTask.h"
#endif //USE_LIBSPE2

#ifdef USE_PARALLEL_SOLVER
#include "BulletMultiThreaded/btParallelConstraintSolver.h"
#include "BulletMultiThreaded/SequentialThreadSupport.h"


std::shared_ptr<btThreadSupportInterface> createSolverThreadSupport(int maxNumThreads)
{
    //#define SEQUENTIAL
#ifdef SEQUENTIAL
	SequentialThreadSupport::SequentialThreadConstructionInfo tci("solverThreads",SolverThreadFunc,SolverlsMemoryFunc);
	SequentialThreadSupport* threadSupport = new SequentialThreadSupport(tci);
	threadSupport->startSPU();
#else
    
#ifdef _WIN32
	Win32ThreadSupport::Win32ThreadConstructionInfo threadConstructionInfo("solverThreads",SolverThreadFunc,SolverlsMemoryFunc,maxNumThreads);
	Win32ThreadSupport* threadSupport = new Win32ThreadSupport(threadConstructionInfo);
	threadSupport->startSPU();
#elif defined (USE_PTHREADS)
	PosixThreadSupport::ThreadConstructionInfo solverConstructionInfo("solver", SolverThreadFunc,
																	  SolverlsMemoryFunc, maxNumThreads);
	
	PosixThreadSupport* threadSupport = new PosixThreadSupport(solverConstructionInfo);
	
#else
	SequentialThreadSupport::SequentialThreadConstructionInfo tci("solverThreads",SolverThreadFunc,SolverlsMemoryFunc);
	SequentialThreadSupport* threadSupport = new SequentialThreadSupport(tci);
	threadSupport->startSPU();
#endif
	
#endif
    
	return std::shared_ptr<btThreadSupportInterface>(threadSupport);
}
#endif //USE_PARALLEL_SOLVER
#endif//USE_PARALLEL_DISPATCHER

using namespace std;

namespace kinski{ namespace physics{
    
    btCollisionShapePtr createCollisionShape(const gl::GeometryPtr &geom)
    {
        btStridingMeshInterface *striding_mesh = new physics::Geometry(geom);
        return physics::btCollisionShapePtr(new btBvhTriangleMeshShape(striding_mesh, false));
    }
    
    physics_context::~physics_context()
    {
        teardown_physics();
        deleteCollisionLocalStoreMemory();
    }
    
    void physics_context::initPhysics()
    {
        LOG_INFO<<"initializing physics";
        
        ///collision configuration contains default setup for memory, collision setup
        btDefaultCollisionConstructionInfo cci;
        if(m_maxNumTasks > 1)
            cci.m_defaultMaxPersistentManifoldPoolSize = 32768;
        m_collisionConfiguration = shared_ptr<btDefaultCollisionConfiguration>(
                                                                               new btDefaultCollisionConfiguration());
        
        //m_collisionConfiguration->setConvexConvexMultipointIterations();
        
        ///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
        m_dispatcher = shared_ptr<btCollisionDispatcher>(new btCollisionDispatcher(m_collisionConfiguration.get()));
        
        m_broadphase = shared_ptr<btBroadphaseInterface>(new btDbvtBroadphase());
        
        ///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
        m_solver = shared_ptr<btConstraintSolver>(new btSequentialImpulseConstraintSolver);
        
        if(m_maxNumTasks > 1)
        {
            PosixThreadSupport::ThreadConstructionInfo constructionInfo("collision",
                                                                        processCollisionTask,
                                                                        createCollisionLocalStoreMemory,
                                                                        m_maxNumTasks);
            m_threadSupportCollision = std::shared_ptr<btThreadSupportInterface>(new PosixThreadSupport(constructionInfo));
            m_dispatcher = shared_ptr<btCollisionDispatcher>(new SpuGatheringCollisionDispatcher(m_threadSupportCollision.get(),
                                                                                                 m_maxNumTasks,
                                                                                                 m_collisionConfiguration.get()));
            
            m_threadSupportSolver = createSolverThreadSupport(m_maxNumTasks);
            m_solver = shared_ptr<btConstraintSolver>(new btParallelConstraintSolver(m_threadSupportSolver.get()));
            //this solver requires the contacts to be in a contiguous pool, so avoid dynamic allocation
            m_dispatcher->setDispatcherFlags(btCollisionDispatcher::CD_DISABLE_CONTACTPOOL_DYNAMIC_ALLOCATION);
        }
        
        m_dynamicsWorld = shared_ptr<btDynamicsWorld>(new btDiscreteDynamicsWorld(m_dispatcher.get(),
                                                                                  m_broadphase.get(),
                                                                                  m_solver.get(),
                                                                                  m_collisionConfiguration.get()));

        if(m_maxNumTasks > 1)
        {
            btDiscreteDynamicsWorld* world = (btDiscreteDynamicsWorld*) m_dynamicsWorld.get();
            world->getSimulationIslandManager()->setSplitIslands(false);
            world->getSolverInfo().m_numIterations = 4;
            world->getSolverInfo().m_solverMode = SOLVER_SIMD+SOLVER_USE_WARMSTARTING;//+SOLVER_RANDMIZE_ORDER;
            world->getDispatchInfo().m_enableSPU = true;
        }
        
        m_dynamicsWorld->setGravity(btVector3(0,-500,0));
        
        //////////////////////////////////////////////////////////////
        
        //boost::bind(&physics_context::near_callback, this);
        //m_dynamicsWorld->getPairCache()->setOverlapFilterCallback(filterCallback);
    }
    
    void physics_context::teardown_physics()
    {
        int i;
        for (i = m_dynamicsWorld->getNumCollisionObjects()-1; i>=0 ;i--)
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
    
    void physics_context::near_callback(btBroadphasePair& collisionPair, btCollisionDispatcher& dispatcher,
                                        btDispatcherInfo& dispatchInfo)
    {
    
    }
/********************** BulletGeometry (btStridingMeshInterface implementation) *******************/
    
    Geometry::Geometry(const gl::GeometryPtr &the_geom):
    btStridingMeshInterface(),
    m_geometry(the_geom)
    {
        if(!the_geom) throw Exception("tried to init a BulletGeometry from NULL pointer");
    }
    
    void Geometry::getLockedVertexIndexBase(unsigned char **vertexbase,
                                                  int& numverts,
                                                  PHY_ScalarType& type,
                                                  int& stride,
                                                  unsigned char **indexbase,
                                                  int & indexstride,
                                                  int& numfaces,
                                                  PHY_ScalarType& indicestype,
                                                  int subpart)
    {
        vertexbase[subpart] = reinterpret_cast<unsigned char*>(&m_geometry->vertices()[0]);
        numverts = m_geometry->vertices().size();
        type = PHY_FLOAT;
        stride = sizeof(m_geometry->vertices()[0]);
        indexbase[subpart] = reinterpret_cast<unsigned char*>(&m_geometry->indices()[0]);
        indexstride = 3 * sizeof(m_geometry->indices()[0]);
        numfaces = m_geometry->faces().size();
        indicestype = PHY_INTEGER;
    }
    
    void Geometry::getLockedReadOnlyVertexIndexBase(const unsigned char **vertexbase,
                                                          int& numverts,
                                                          PHY_ScalarType& type,
                                                          int& stride,
                                                          const unsigned char **indexbase,
                                                          int & indexstride,
                                                          int& numfaces,
                                                          PHY_ScalarType& indicestype,
                                                          int subpart) const
    {
        vertexbase[subpart] = reinterpret_cast<const unsigned char*>(&m_geometry->vertices()[0]);
        numverts = m_geometry->vertices().size();
        type = PHY_FLOAT;
        stride = sizeof(m_geometry->vertices()[0]);
        indexbase[subpart] = reinterpret_cast<const unsigned char*>(&m_geometry->indices()[0]);
        indexstride = 3 * sizeof(m_geometry->indices()[0]);
        numfaces = m_geometry->faces().size();
        indicestype = PHY_INTEGER;
    }
    
    /// unLockVertexBase finishes the access to a subpart of the triangle mesh
    /// make a call to unLockVertexBase when the read and write access (using getLockedVertexIndexBase) is finished
    void Geometry::unLockVertexBase(int subpart)
    {
    
    }
    
    void Geometry::unLockReadOnlyVertexBase(int subpart) const
    {
    
    }
    
    
    /// getNumSubParts returns the number of seperate subparts
    /// each subpart has a continuous array of vertices and indices
    int Geometry::getNumSubParts() const
    {
        return 1;
    }
    
    void Geometry::preallocateVertices(int numverts)
    {
        m_geometry->vertices().resize(numverts);
    }
    
    void Geometry::preallocateIndices(int numindices)
    {
        m_geometry->indices().resize(numindices);
    }
    
/**************************************************************************************************/
    
    
    
//    void MyNearCallback(btBroadphasePair& collisionPair, btCollisionDispatcher& dispatcher,
//                        btDispatcherInfo& dispatchInfo)
//    {
//        // Do your collision logic here
//        // Only dispatch the Bullet collision information if you want the physics to continue
//        dispatcher.defaultNearCallback(collisionPair, dispatcher, dispatchInfo);
//    }
}}
