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
    
    btCollisionShapePtr createCollisionShape(const gl::MeshPtr &the_mesh, const glm::vec3 &the_scale)
    {
        btStridingMeshInterface *striding_mesh = new physics::Mesh(the_mesh);
        striding_mesh->setScaling(type_cast(the_scale));
        return physics::btCollisionShapePtr(new btBvhTriangleMeshShape(striding_mesh, false));
    }
    
    btCollisionShapePtr createConvexCollisionShape(const gl::MeshPtr &the_mesh,
                                                   const glm::vec3 &the_scale)
    {
        const vector<glm::vec3> &vertices = the_mesh->geometry()->vertices();
        physics::btCollisionShapePtr hull_shape(new btConvexHullShape((btScalar*)&vertices[0],
                                                                      vertices.size(),
                                                                      sizeof(vertices[0])));
        hull_shape->setLocalScaling(type_cast(the_scale));
        return hull_shape;
    }
    
    
    btVector3 type_cast(const glm::vec3 &the_vec)
    {
        return btVector3(the_vec[0], the_vec[1], the_vec[2]);
    }
    
    btTransform type_cast(const glm::mat4 &the_transform)
    {
        btTransform ret;
        ret.setFromOpenGLMatrix(&the_transform[0][0]);
        return ret;
    }
    
    glm::vec3 type_cast(const btVector3 &the_vec)
    {
        return glm::vec3(the_vec[0], the_vec[1], the_vec[2]);
    }
    
    glm::mat4 type_cast(const btTransform &the_transform)
    {
        float m[16];
        the_transform.getOpenGLMatrix(m);
        return glm::make_mat4(m);
    }
    
    physics_context::~physics_context()
    {
        teardown_physics();
    }
    
    void physics_context::initPhysics()
    {
        LOG_DEBUG<<"initializing physics";
        boost::mutex::scoped_lock lock(m_mutex);
        
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
    }
    
    void physics_context::stepPhysics(float timestep)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        if(m_dynamicsWorld)
            m_dynamicsWorld->stepSimulation(timestep, 1);
    }
    
    void physics_context::teardown_physics()
    {
        boost::mutex::scoped_lock lock(m_mutex);
        if(!m_dynamicsWorld) return;
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
        deleteCollisionLocalStoreMemory();
    }
    
    void physics_context::near_callback(btBroadphasePair& collisionPair, btCollisionDispatcher& dispatcher,
                                        btDispatcherInfo& dispatchInfo)
    {
    
    }
    
    void physics_context::add_mesh_to_simulation(const gl::MeshPtr &the_mesh)
    {
        btCollisionShapePtr customShape = createCollisionShape(the_mesh, the_mesh->scale());
        m_collisionShapes.push_back(customShape);
        physics::MotionState *ms = new physics::MotionState(the_mesh);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(0.f, ms, customShape.get());
        btRigidBody* body = new btRigidBody(rbInfo);
        body->setFriction(0.1f);
        body->setRestitution(0.1f);
        body->setCollisionFlags( body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        body->setActivationState(DISABLE_DEACTIVATION);
        
        //add the body to the dynamics world
        m_dynamicsWorld->addRigidBody(body);
    }
    
/***************** kinski::physics::Mesh (btStridingMeshInterface implementation) *****************/
    
    Mesh::Mesh(const gl::MeshPtr &the_mesh):
    btStridingMeshInterface(),
    m_mesh(the_mesh)
    {
        if(!the_mesh) throw Exception("tried to init a physics::Mesh from empty gl::Mesh");
    }
    
    void Mesh::getLockedVertexIndexBase(unsigned char **vertexbase,
                                        int& numverts,
                                        PHY_ScalarType& type,
                                        int& stride,
                                        unsigned char **indexbase,
                                        int & indexstride,
                                        int& numfaces,
                                        PHY_ScalarType& indicestype,
                                        int subpart)
    {
        gl::GeometryPtr &geom = m_mesh->geometry();
        gl::Mesh::Entry& e = m_mesh->entries()[subpart];
        *vertexbase = reinterpret_cast<unsigned char*>(&geom->vertices()[e.base_vertex]);
        numverts = e.num_vertices;
        type = PHY_FLOAT;
        stride = sizeof(geom->vertices()[0]);
        *indexbase = reinterpret_cast<unsigned char*>(&geom->indices()[e.base_index]);
        indexstride = 3 * sizeof(geom->indices()[0]);
        numfaces = e.numdices / 3;
        indicestype = PHY_INTEGER;
    }
    
    void Mesh::getLockedReadOnlyVertexIndexBase(const unsigned char **vertexbase,
                                                int& numverts,
                                                PHY_ScalarType& type,
                                                int& stride,
                                                const unsigned char **indexbase,
                                                int & indexstride,
                                                int& numfaces,
                                                PHY_ScalarType& indicestype,
                                                int subpart) const
    {
        gl::GeometryPtr &geom = m_mesh->geometry();
        gl::Mesh::Entry& e = m_mesh->entries()[subpart];
        *vertexbase = reinterpret_cast<const unsigned char*>(&geom->vertices()[e.base_vertex]);
        numverts = e.num_vertices;
        type = PHY_FLOAT;
        stride = sizeof(geom->vertices()[0]);
        *indexbase = reinterpret_cast<const unsigned char*>(&geom->indices()[e.base_index]);
        indexstride = 3 * sizeof(geom->indices()[0]);
        numfaces = e.numdices / 3;
        indicestype = PHY_INTEGER;
    }
    
    /// unLockVertexBase finishes the access to a subpart of the triangle mesh
    /// make a call to unLockVertexBase when the read and write access (using getLockedVertexIndexBase) is finished
    void Mesh::unLockVertexBase(int subpart)
    {
    
    }
    
    void Mesh::unLockReadOnlyVertexBase(int subpart) const
    {
    
    }
    
    
    /// getNumSubParts returns the number of seperate subparts
    /// each subpart has a continuous array of vertices and indices
    int Mesh::getNumSubParts() const
    {
        return m_mesh->entries().size();
    }
    
    void Mesh::preallocateVertices(int numverts)
    {
        m_mesh->geometry()->vertices().resize(numverts);
    }
    
    void Mesh::preallocateIndices(int numindices)
    {
        m_mesh->geometry()->indices().resize(numindices);
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
