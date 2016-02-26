//
//  physics_context.h
//  gl
//
//  Created by Fabian on 1/16/13.
//
//

#pragma once

#include "core/core.hpp"
#include <unordered_map>

//#define BT_USE_DOUBLE_PRECISION
#include "btBulletDynamicsCommon.h"

#include "gl/Mesh.hpp"

class btThreadSupportInterface;

namespace kinski{ namespace physics{
    
    typedef std::shared_ptr<btCollisionShape> btCollisionShapePtr;
    typedef std::shared_ptr<const btCollisionShape> btCollisionShapeConstPtr;
    typedef std::shared_ptr<btCollisionObject> btCollisionObjectPtr;
    typedef std::shared_ptr<const btCollisionObject> btCollisionConstObjectPtr;
    typedef std::shared_ptr<btDynamicsWorld> btDynamicsWorldPtr;
    typedef std::shared_ptr<const btDynamicsWorld> btDynamicsWorldConstPtr;
    typedef std::shared_ptr<btIDebugDraw> btIDebugDrawPtr;
    
    KINSKI_API btVector3 type_cast(const glm::vec3 &the_vec);
    KINSKI_API btTransform type_cast(const glm::mat4 &the_transform);
    KINSKI_API glm::vec3 type_cast(const btVector3 &the_vec);
    KINSKI_API glm::mat4 type_cast(const btTransform &the_transform);
    KINSKI_API btCollisionShapePtr createCollisionShape(const gl::MeshPtr &the_mesh,
                                                        const glm::vec3 &the_scale = glm::vec3(1));
    KINSKI_API btCollisionShapePtr createConvexCollisionShape(const gl::MeshPtr &the_mesh,
                                                              const glm::vec3 &the_scale = glm::vec3(1));
    
    KINSKI_API class BulletDebugDrawer : public btIDebugDraw
    {
    public:
        
        BulletDebugDrawer()
        {
            gl::MaterialPtr mat = gl::Material::create();
            gl::GeometryPtr geom = gl::Geometry::create();
            m_mesh_lines = gl::Mesh::create(geom, mat);
            m_mesh_lines->geometry()->setPrimitiveType(GL_LINES);
//            setDebugMode(getDebugMode() | DBG_DrawConstraints);
        };
        
        inline void drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
        {
            m_mesh_lines->geometry()->appendVertex(glm::vec3(from.x(), from.y(), from.z()));
            m_mesh_lines->geometry()->appendVertex(glm::vec3(to.x(), to.y(), to.z()));
            m_mesh_lines->geometry()->appendColor(glm::vec4(color.x(), color.y(), color.z(), 1.0f));
            m_mesh_lines->geometry()->appendColor(glm::vec4(color.x(), color.y(), color.z(), 1.0f));
        }
        
        void drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,
                              btScalar distance,int lifeTime,const btVector3& color){};
        
        void reportErrorWarning(const char* warningString) {LOG_WARNING<<warningString;}
        void draw3dText(const btVector3& location,const char* textString)
        {
            //TODO: font rendering here
        }
        void setDebugMode(int debugMode){ LOG_WARNING << "unsupported operation"; }
        int	getDebugMode() const {return DBG_DrawWireframe;}
        
        //!
        // issue the actual draw command
        inline void flush()
        {
            m_mesh_lines->geometry()->createGLBuffers();
            
            gl::draw_mesh(m_mesh_lines);
            m_mesh_lines->geometry()->vertices().clear();
            m_mesh_lines->geometry()->colors().clear();
            m_mesh_lines->geometry()->vertices().reserve(1024);
            m_mesh_lines->geometry()->colors().reserve(1024);
        };
        
    private:
        gl::MeshPtr m_mesh_lines, m_mesh_points;
    };
    
    
    KINSKI_API ATTRIBUTE_ALIGNED16(struct)	MotionState : public btMotionState
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
            m_object->setTransform(transform);
        }
    };
    
    typedef std::shared_ptr<class Mesh> MeshPtr;
    
    KINSKI_API class Mesh : public btStridingMeshInterface
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
    
    KINSKI_API class physics_context
    {
     public:
        
        explicit physics_context(int num_tasks = 1):m_maxNumTasks(num_tasks){};
        ~physics_context();
        
        void init();
        void step_simulation(float timestep, int max_sub_steps = 1, float fixed_time_step = 1.f / 60.f);
        void debug_render(gl::CameraPtr the_cam);
        void teardown();
        
        const btDynamicsWorldConstPtr dynamicsWorld() const {return m_dynamicsWorld;};
        const btDynamicsWorldPtr& dynamicsWorld() {return m_dynamicsWorld;};
        
        const std::vector<btRigidBody*>& bounding_bodies() const { return m_bounding_bodies; };
        std::vector<btRigidBody*>& bounding_bodies() { return m_bounding_bodies; };
        
        const std::set<btCollisionShapePtr>& collisionShapes() const {return m_collisionShapes;};
        std::set<btCollisionShapePtr>& collisionShapes() {return m_collisionShapes;};
        
        void near_callback(btBroadphasePair& collisionPair, btCollisionDispatcher& dispatcher,
                           btDispatcherInfo& dispatchInfo);
        
        /*!
         * Add a kinski::MeshPtr instance to the physics simulation,
         * with an optional collision shape.
         * If no collision shape is provided, a (static) btBvhTriangleMeshShape instance will be created.
         * return: A pointer to the newly created btRigidBody
         */
        btRigidBody* add_mesh_to_simulation(const gl::MeshPtr &the_mesh, float mass = 0.f,
                                            btCollisionShapePtr col_shape = btCollisionShapePtr());
        
        bool remove_mesh_from_simulation(const gl::MeshPtr &the_mesh);
        
        /*!
         * return a pointer to the corresponding btRigidBody for the_mesh
         * or nullptr if not found
         */
        btRigidBody* get_rigidbody_for_mesh(const gl::MeshPtr &the_mesh);
        
        /*!
         * set where to position static planes as boundaries for the entire physics scene
         */
        void set_world_boundaries(const glm::vec3 &the_half_extents,
                                  const glm::vec3 &the_origin = glm::vec3(0));
        
        void attach_constraints(float the_thresh);
        
        /*!
         * internal tick callback, do not call directly
         */
        void tick_callback(btScalar timeStep);
        
     private:
        
        std::unordered_map<gl::MeshPtr, btCollisionShapePtr> m_mesh_shape_map;
        std::unordered_map<gl::MeshPtr, btRigidBody*> m_mesh_rigidbody_map;
        
        std::set<btCollisionShapePtr> m_collisionShapes;
        std::shared_ptr<btBroadphaseInterface> m_broadphase;
        std::shared_ptr<btCollisionDispatcher> m_dispatcher;
        std::shared_ptr<btConstraintSolver> m_solver;
        std::shared_ptr<btDefaultCollisionConfiguration> m_collisionConfiguration;
        btDynamicsWorldPtr m_dynamicsWorld;
        
        uint32_t m_maxNumTasks;
        std::shared_ptr<btThreadSupportInterface> m_threadSupportCollision;
        std::shared_ptr<btThreadSupportInterface> m_threadSupportSolver;
        
        std::shared_ptr<BulletDebugDrawer> m_debug_drawer;
        
        std::vector<btCollisionShapePtr> m_bounding_shapes;
        std::vector<btRigidBody*> m_bounding_bodies;
    };
    
    //////////////////////////////// fracture utilities ////////////////////////////////////////////
    
    struct VoronoiShard
    {
        gl::MeshPtr mesh;
        float volume;
    };
    
    std::list<VoronoiShard>
    voronoi_convex_hull_shatter(const gl::MeshPtr &the_mesh,
                                const std::vector<glm::vec3>& the_voronoi_points);
    
}}//namespace