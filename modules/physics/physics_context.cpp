//
//  physics_context.cpp
//  gl
//
//  Created by Fabian on 1/16/13.
//
//

//#include <boost/bind.hpp>
#include "physics_context.h"

#include "btBulletDynamicsCommon.h"
#include "BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btSimulationIslandManager.h"

using namespace std;

namespace kinski{ namespace physics{
    
    typedef std::shared_ptr<class Mesh> MeshPtr;
    
    class Mesh : public btStridingMeshInterface
    {
    public:
        Mesh(const gl::MeshPtr &the_mesh);
        
        /// get read and write access to a subpart of a triangle mesh
        /// this subpart has a continuous array of vertices and indices
        /// in this way the mesh can be handled as chunks of memory with striding
        /// very similar to OpenGL vertexarray support
        /// make a call to unLockVertexBase when the read and write access is finished
        void getLockedVertexIndexBase(unsigned char **vertexbase,
                                      int& numverts,PHY_ScalarType& type,
                                      int& stride,unsigned char **indexbase,
                                      int & indexstride,int& numfaces,
                                      PHY_ScalarType& indicestype,
                                      int subpart=0);
        
        void getLockedReadOnlyVertexIndexBase(const unsigned char **vertexbase,
                                              int& numverts,PHY_ScalarType& type,
                                              int& stride,
                                              const unsigned char **indexbase,
                                              int & indexstride,
                                              int& numfaces,
                                              PHY_ScalarType& indicestype,
                                              int subpart=0) const;
        
        /// unLockVertexBase finishes the access to a subpart of the triangle mesh
        /// make a call to unLockVertexBase when the read and write access (using getLockedVertexIndexBase) is finished
        void unLockVertexBase(int subpart);
        
        void unLockReadOnlyVertexBase(int subpart) const;
        
        
        /// getNumSubParts returns the number of seperate subparts
        /// each subpart has a continuous array of vertices and indices
        int getNumSubParts() const;
        
        void preallocateVertices(int numverts);
        void preallocateIndices(int numindices);
        
    private:
        
        gl::MeshPtr m_mesh;
    };
    
    ATTRIBUTE_ALIGNED16(struct)	MotionState : public btMotionState
    {
        gl::Object3DPtr m_object;
        btTransform m_graphicsWorldTrans;
        btTransform	m_centerOfMassOffset;
        BT_DECLARE_ALIGNED_ALLOCATOR();
        
        MotionState(const gl::Object3DPtr& theObject3D,
                    const btTransform& centerOfMassOffset = btTransform::getIdentity()):
        m_object(theObject3D),
        m_centerOfMassOffset(centerOfMassOffset)
        {
            // remove scale from transformation matrix, bullet expects unscaled transforms
            glm::vec3 scale = theObject3D->scale();
            glm::mat4 transform = glm::scale(theObject3D->transform(), 1.f / scale);
            m_graphicsWorldTrans.setFromOpenGLMatrix(&transform[0][0]);
        }
        
        ///synchronizes world transform from user to physics
        virtual void getWorldTransform(btTransform& centerOfMassWorldTrans ) const
        {
            // remove scale from transformation matrix, bullet expects unscaled transforms
            glm::mat4 transform = glm::scale(m_object->transform(), 1.f / m_object->scale());
            btTransform t;
            t.setFromOpenGLMatrix(&transform[0][0]);
            centerOfMassWorldTrans = m_centerOfMassOffset.inverse() * t ;
        }
        
        ///synchronizes world transform from physics to user
        ///Bullet only calls the update of worldtransform for active objects
        virtual void setWorldTransform(const btTransform& centerOfMassWorldTrans)
        {
            m_graphicsWorldTrans = centerOfMassWorldTrans * m_centerOfMassOffset ;
            glm::mat4 transform;
            m_graphicsWorldTrans.getOpenGLMatrix(&transform[0][0]);
            transform = glm::scale(transform, m_object->scale());
            m_object->set_transform(transform);
        }
    };
    
    /*
     * subclass of btBvhTriangleMeshShape,
     * which encapsulates a physics::MeshPtr (btStridingMeshInterface)
     */
    class TriangleMeshShape : public btBvhTriangleMeshShape
    {
    public:
        
        TriangleMeshShape(physics::MeshPtr meshInterface,
                          bool useQuantizedAabbCompression,
                          bool buildBvh = true):
        btBvhTriangleMeshShape(meshInterface.get(), useQuantizedAabbCompression, buildBvh),
        m_striding_mesh(meshInterface)
        {}
        
        ///optionally pass in a larger bvh aabb, used for quantization. This allows for deformations within this aabb
        TriangleMeshShape(physics::MeshPtr meshInterface,
                          bool useQuantizedAabbCompression,
                          const btVector3& bvhAabbMin,
                          const btVector3& bvhAabbMax,
                          bool buildBvh = true):
        btBvhTriangleMeshShape(meshInterface.get(), useQuantizedAabbCompression, bvhAabbMin, bvhAabbMax, buildBvh),
        m_striding_mesh(meshInterface)
        {}
        
    private:
        physics::MeshPtr m_striding_mesh;
    };
    
    btCollisionShapePtr create_collision_shape(const gl::MeshPtr &the_mesh, const glm::vec3 &the_scale)
    {
        auto phy_mesh = std::make_shared<physics::Mesh>(the_mesh);
        phy_mesh->setScaling(type_cast(the_scale));
        return std::make_shared<TriangleMeshShape>(phy_mesh, true);
    }
    
    btCollisionShapePtr create_convex_collision_shape(const gl::MeshPtr &the_mesh,
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
        btScalar m[16];
        the_transform.getOpenGLMatrix(m);
        return glm::make_mat4(m);
    }
    
    void _tick_callback(btDynamicsWorld *world, btScalar timeStep)
    {
        physics_context *p = static_cast<physics_context*>(world->getWorldUserInfo());
        p->tick_callback(timeStep);
    }
    
    physics_context::~physics_context()
    {
        teardown();
    }
    
    void physics_context::init()
    {
        teardown();
        
        LOG_DEBUG<<"initializing physics";
        
        ///collision configuration contains default setup for memory, collision setup
        btDefaultCollisionConstructionInfo cci;
        if(m_maxNumTasks > 1){ cci.m_defaultMaxPersistentManifoldPoolSize = 32768; }
        
        m_collisionConfiguration = std::make_shared<btDefaultCollisionConfiguration>();
        
        ///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
        m_dispatcher = std::make_shared<btCollisionDispatcher>(m_collisionConfiguration.get());
        
        m_broadphase = std::make_shared<btDbvtBroadphase>();
        
        ///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
        m_solver = std::make_shared<btSequentialImpulseConstraintSolver>();
        m_dynamicsWorld = std::make_shared<btDiscreteDynamicsWorld>(m_dispatcher.get(),
                                                                    m_broadphase.get(),
                                                                    m_solver.get(),
                                                                    m_collisionConfiguration.get());

        m_dynamicsWorld->setGravity(btVector3(0,-9.87,0));
        
        // debug drawer
        m_debug_drawer = std::make_shared<BulletDebugDrawer>();
        m_dynamicsWorld->setDebugDrawer(m_debug_drawer.get());
        
        // tick callback
        m_dynamicsWorld->setInternalTickCallback(&_tick_callback, static_cast<void*>(this));
    }
    
    void physics_context::step_simulation(float timestep, int max_sub_steps, float fixed_time_step)
    {
        if(m_dynamicsWorld)
            m_dynamicsWorld->stepSimulation(timestep, max_sub_steps, fixed_time_step);
    }
    
    void physics_context::teardown()
    {
        if(!m_dynamicsWorld) return;
        
        //cleanup in the reverse order of creation/initialization
        
        int i;
        //remove all constraints
        for (i = m_dynamicsWorld->getNumConstraints() - 1; i >= 0; i--)
        {
            btTypedConstraint* constraint = m_dynamicsWorld->getConstraint(i);
            m_dynamicsWorld->removeConstraint(constraint);
            delete constraint;
        }
        
        //remove the rigidbodies from the dynamics world and delete them
        
        for (i = m_dynamicsWorld->getNumCollisionObjects()- 1; i>= 0; i--)
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
        
        m_bounding_bodies.clear();
        m_bounding_shapes.clear();
        m_collisionShapes.clear();
        m_mesh_shape_map.clear();
        m_mesh_rigidbody_map.clear();
    }
    
    void physics_context::near_callback(btBroadphasePair& collisionPair, btCollisionDispatcher& dispatcher,
                                        btDispatcherInfo& dispatchInfo)
    {
    
    }
    
    void physics_context::debug_render(gl::CameraPtr the_cam)
    {
        gl::ScopedMatrixPush mv(gl::MODEL_VIEW_MATRIX), p(gl::PROJECTION_MATRIX);
        gl::set_matrices(the_cam);
        
        if(m_dynamicsWorld)
        {
            m_dynamicsWorld->debugDrawWorld();
            m_debug_drawer->flush();
        }
    }
    
    btRigidBody* physics_context::get_rigidbody_for_mesh(const gl::MeshPtr &the_mesh)
    {
        auto body_iter = m_mesh_rigidbody_map.find(the_mesh);
        
        if(body_iter != m_mesh_rigidbody_map.end()){ return body_iter->second; }
        return nullptr;
    }
    
    btRigidBody* physics_context::add_mesh_to_simulation(const gl::MeshPtr &the_mesh, float mass,
                                                         btCollisionShapePtr col_shape)
    {
        if(!m_dynamicsWorld || !the_mesh) return nullptr;
        
        // look for an existing col_shape for this mesh
        auto iter = m_mesh_shape_map.find(the_mesh);
        auto body_iter = m_mesh_rigidbody_map.find(the_mesh);
        
        // already added to simulation
        if(body_iter != m_mesh_rigidbody_map.end())
        {
            LOG_WARNING << "mesh already added to simulation";
            
            // remove the rigidbody from the simulation
            btRigidBody *rb = body_iter->second;
            if (m_dynamicsWorld && rb && rb->getMotionState())
            {
                m_dynamicsWorld->removeCollisionObject(rb);
                delete rb->getMotionState();
                delete rb;
            }
        }
        
        if(iter == m_mesh_shape_map.end())
        {
            if(!col_shape){ col_shape = create_collision_shape(the_mesh, the_mesh->scale()); }
            m_mesh_shape_map[the_mesh] = col_shape;
            m_collisionShapes.insert(col_shape);
        }
        else
        {
            if(col_shape)
            {
                // remove old collision shape from set
                m_collisionShapes.erase(m_collisionShapes.find(iter->second));
                m_collisionShapes.insert(col_shape);
                m_mesh_shape_map[the_mesh] = col_shape;
            }
            else
            {
                col_shape = iter->second;
            }
        }
        physics::MotionState *ms = new physics::MotionState(the_mesh);
        btVector3 localInertia;
        if(mass != 0.f)
            col_shape->calculateLocalInertia(mass, localInertia);
        
        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, ms, col_shape.get(), localInertia);
        btRigidBody* body = new btRigidBody(rbInfo);
        
        //add the body to the dynamics world
        m_dynamicsWorld->addRigidBody(body);
        m_mesh_rigidbody_map[the_mesh] = body;
        return body;
    }
    
    bool physics_context::remove_mesh_from_simulation(const gl::MeshPtr &the_mesh)
    {
        btRigidBody *rb = get_rigidbody_for_mesh(the_mesh);

        if(!rb){ return false; }
        if (m_dynamicsWorld)
        {
            m_dynamicsWorld->removeCollisionObject(rb);
            if(rb->getMotionState()){ delete rb->getMotionState(); }
            delete rb;
        }
        
        return true;
    }
    
    void physics_context::set_world_boundaries(const glm::vec3 &the_half_extents,
                                               const glm::vec3 &the_origin)
    {
        // add static plane boundaries
        physics::btCollisionShapePtr
        ground_plane (new btStaticPlaneShape(btVector3(0, 1, 0), the_origin[1] - the_half_extents[1])),
        front_plane(new btStaticPlaneShape(btVector3(0, 0, -1), -the_origin[2] - the_half_extents[2])),
        back_plane(new btStaticPlaneShape(btVector3(0, 0, 1), the_origin[2] - the_half_extents[2])),
        left_plane(new btStaticPlaneShape(btVector3(1, 0, 0), the_origin[0] - the_half_extents[0])),
        right_plane(new btStaticPlaneShape(btVector3(-1, 0, 0), -the_origin[0] - the_half_extents[0])),
        top_plane(new btStaticPlaneShape(btVector3(0, -1, 0), -the_origin[1] - the_half_extents[1]));
        
        m_bounding_shapes = {ground_plane, front_plane, back_plane, left_plane, right_plane, top_plane};
        
        // remove previous bounding bodies
        for(btRigidBody *rb : m_bounding_bodies)
        {        
            if (m_dynamicsWorld && rb)
            {
                m_dynamicsWorld->removeRigidBody(rb);
                delete rb;
            }
        }
        m_bounding_bodies.clear();
        
        for (const auto &shape : m_bounding_shapes)
        {
            btRigidBody::btRigidBodyConstructionInfo rbInfo(0.f, nullptr, shape.get());
            btRigidBody* body = new btRigidBody(rbInfo);
            body->setFriction(.7f);
            body->setRestitution(0);
//            body->setCollisionFlags( body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
//            body->setActivationState(DISABLE_DEACTIVATION);
            
            //add the body to the dynamics world
            m_dynamicsWorld->addRigidBody(body);
            m_bounding_bodies.push_back(body);
        }
    }
    
    void physics_context::attach_constraints(float the_thresh)
    {
        btAlignedObjectArray<btRigidBody*> bodies;
        
        int numManifolds = m_dynamicsWorld->getDispatcher()->getNumManifolds();
        
        for (int i=0;i<numManifolds;i++)
        {
            btPersistentManifold* manifold = m_dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
            if (!manifold->getNumContacts())
                continue;
            
            btScalar minDist = 1e30f;
            int minIndex = -1;
            for (int v=0;v<manifold->getNumContacts();v++)
            {
                if (minDist >manifold->getContactPoint(v).getDistance())
                {
                    minDist = manifold->getContactPoint(v).getDistance();
                    minIndex = v;
                }
            }
            if (minDist>0.)
                continue;
            
            btCollisionObject* colObj0 = (btCollisionObject*)manifold->getBody0();
            btCollisionObject* colObj1 = (btCollisionObject*)manifold->getBody1();
            //	int tag0 = (colObj0)->getIslandTag();
            //		int tag1 = (colObj1)->getIslandTag();
            btRigidBody* body0 = btRigidBody::upcast(colObj0);
            btRigidBody* body1 = btRigidBody::upcast(colObj1);
            if (bodies.findLinearSearch(body0)==bodies.size())
                bodies.push_back(body0);
            if (bodies.findLinearSearch(body1)==bodies.size())
                bodies.push_back(body1);
            
            if (body0 && body1 &&
                !colObj0->isStaticOrKinematicObject() && !colObj1->isStaticOrKinematicObject() &&
                body0->checkCollideWithOverride(body1))
            {
                btTransform trA,trB;
                trA.setIdentity();
                trB.setIdentity();
                btVector3 contactPosWorld = manifold->getContactPoint(minIndex).m_positionWorldOnA;
                btTransform globalFrame;
                globalFrame.setIdentity();
                globalFrame.setOrigin(contactPosWorld);
                
                trA = body0->getWorldTransform().inverse()*globalFrame;
                trB = body1->getWorldTransform().inverse()*globalFrame;
                float totalMass = 1.f/body0->getInvMass() + 1.f/body1->getInvMass();

                btFixedConstraint* fixed = new btFixedConstraint(*body0,*body1,trA,trB);
                fixed->setBreakingImpulseThreshold(the_thresh * totalMass);
                fixed ->setOverrideNumSolverIterations(30);
                m_dynamicsWorld->addConstraint(fixed,true);
            }
        }//for
    }
    
    void physics_context::tick_callback(btScalar timeStep)
    {
        int collisions = 0;
        int numManifolds = m_dynamicsWorld->getDispatcher()->getNumManifolds();
        for (int i = 0; i < numManifolds; i++)
        {
            btPersistentManifold* contactManifold =  m_dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
            const btCollisionObject* obA = contactManifold->getBody0();
            const btCollisionObject* obB = contactManifold->getBody1();
            
            int numContacts = contactManifold->getNumContacts();
            for (int j = 0; j < numContacts; j++)
            {
                btManifoldPoint& pt = contactManifold->getContactPoint(j);
                if (pt.getDistance() < 0.f)
                {
                    const btVector3& ptA = pt.getPositionWorldOnA();
                    const btVector3& ptB = pt.getPositionWorldOnB();
                    const btVector3& normalOnB = pt.m_normalWorldOnB;
                    collisions++;
                }
            }
        }
        
//        LOG_DEBUG << collisions << " collisions";
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
        numfaces = e.num_indices / 3;
        indicestype = geom->index_type() == GL_UNSIGNED_INT ? PHY_INTEGER : PHY_SHORT;
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
        numfaces = e.num_indices / 3;
        indicestype = geom->index_type() == GL_UNSIGNED_INT ? PHY_INTEGER : PHY_SHORT;
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
//        m_mesh->geometry()->vertices().resize(numverts);
    }
    
    void Mesh::preallocateIndices(int numindices)
    {
//        m_mesh->geometry()->indices().resize(numindices);
    }
    
/**************************************************************************************************/
    
}}
