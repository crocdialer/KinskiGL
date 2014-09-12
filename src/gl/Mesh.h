// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef KINSKI_GL_MESH_H_
#define KINSKI_GL_MESH_H_

#include "Object3D.h"
#include "Geometry.h"
#include "Material.h"

namespace kinski { namespace gl {
    
    struct Bone
    {
        std::string name;
        glm::mat4 transform;
        glm::mat4 worldtransform;
        glm::mat4 offset;
        uint32_t index;
        BonePtr parent;
        std::list<BonePtr> children;
    };
    
    template<typename T> struct Key
    {
        float time;
        T value;
        
        Key(float t, const T &v):time(t), value(v){};
    };
    
    struct AnimationKeys
    {
        std::vector< Key<glm::vec3> > positionkeys;
        std::vector< Key<glm::quat> > rotationkeys;
        std::vector< Key<glm::vec3> > scalekeys;
    };
    
    struct Animation
    {
        float current_time;
        float duration;
        float ticksPerSec;
        std::map<BonePtr, AnimationKeys> boneKeys;
        Animation():current_time(0), ticksPerSec(1.0f){};
    };
    
    class KINSKI_API Mesh : public Object3D
    {
    public:
        
        struct Entry
        {
            Entry():num_indices(0), base_vertex(0), base_index(0), material_index(-1), enabled(true){}
            uint32_t num_indices;
            uint32_t num_vertices;
            uint32_t base_vertex;
            uint32_t base_index;
            int material_index;
            bool enabled;
        };
        
        typedef std::shared_ptr<Mesh> Ptr;
        typedef std::shared_ptr<const Mesh> ConstPtr;
        
        static Ptr create(const GeometryPtr &theGeom, const MaterialPtr &theMaterial)
        {
            return Ptr(new Mesh(theGeom, theMaterial));
        }

        virtual ~Mesh();
        
        const GeometryPtr& geometry() const { return m_geometry; };
        GeometryPtr& geometry() { return m_geometry; };
        
        const MaterialPtr& material() const { return m_materials.front(); };
        MaterialPtr& material() { return m_materials.front(); };
        
        void bindVertexPointers(int material_index = 0) const;
        void bind_vertex_array(int i = 0);
        void createVertexArray();
        GLuint vertexArray(int i = 0) const;
        
        void update(float time_delta);
        
        /*!
         * returns an AABB with global transform applied to it
         */
        AABB boundingBox() const;
        
        const std::vector<Entry>& entries() const {return m_entries;};
        std::vector<Entry>& entries() {return m_entries;};
        
        const std::vector<MaterialPtr>& materials() const {return m_materials;};
        std::vector<MaterialPtr>& materials() {return m_materials;};
        
        const std::vector<AnimationPtr>& animations() const { return m_animations; };
        std::vector<AnimationPtr>& animations() { return m_animations; };
        void addAnimation(const AnimationPtr &theAnim) { m_animations.push_back(theAnim); };
        
        std::vector<glm::mat4>& boneMatrices(){ return m_boneMatrices; };
        const std::vector<glm::mat4>& boneMatrices() const { return m_boneMatrices; };
        
        void initBoneMatrices();
        
        BonePtr& rootBone(){ return m_rootBone; };
        const BonePtr& rootBone() const { return m_rootBone; };
        
        uint32_t get_num_bones(const BonePtr &theRoot);
        
        /*!
         * Set the name under which the attribute will be accessible in the shader.
         * Defaults to "a_vertex"
         */
        void setVertexLocationName(const std::string &theName);
        
        /*!
         * Set the name under which the attribute will be accessible in the shader.
         * Defaults to "a_normal"
         */
        void setNormalLocationName(const std::string &theName);
        
        /*!
         * Set the name under which the attribute will be accessible in the shader.
         * Defaults to "a_tangent"
         */
        void setTangentLocationName(const std::string &theName);
        
        /*!
         * Set the name under which the attribute will be accessible in the shader.
         * Defaults to "a_pointSize"
         */
        void setPointSizeLocationName(const std::string &theName);
        
        /*!
         * Set the name under which the attribute will be accessible in the shader.
         * Defaults to "a_texCoord"
         */
        void setTexCoordLocationName(const std::string &theName);
        
        /*!
         * Set the name under which the attribute will be accessible in the shader.
         * Defaults to "a_color"
         */
        void setColorLocationName(const std::string &theName);
        
        virtual void accept(Visitor &theVisitor);
        
    private:
        
        Mesh(const Geometry::Ptr &theGeom, const Material::Ptr &theMaterial);
        
        void buildBoneMatrices(float time, BonePtr bone,
                               glm::mat4 parentTransform,
                               std::vector<glm::mat4> &matrices);
        
        GeometryPtr m_geometry;
        std::vector<Entry> m_entries;
        std::vector<MaterialPtr> m_materials;
        std::vector<GLuint> m_vertexArrays;
        
        std::vector<gl::Shader> m_shaders;
        
        // skeletal animations stuff
        BonePtr m_rootBone;
        uint32_t m_animation_index;
        std::vector<AnimationPtr> m_animations;
        std::vector<glm::mat4> m_boneMatrices;
        
        std::string m_vertexLocationName;
        std::string m_normalLocationName;
        std::string m_tangentLocationName;
        std::string m_pointSizeLocationName;
        std::string m_texCoordLocationName;
        std::string m_colorLocationName;
        std::string m_boneIDsLocationName;
        std::string m_boneWeightsLocationName;
    };
    
    class WrongVertexArrayDefinedException : public kinski::Exception
    {
    public:
        WrongVertexArrayDefinedException(uint32_t theID):
        Exception("wrong vertex array defined for object: " + kinski::as_string(theID)){}
    };
    
    class SelectMeshVisitor : public Visitor
    {
    public:
        SelectMeshVisitor(){};
        virtual void visit(gl::Mesh &theNode)
        {
            m_objects.push_back(std::dynamic_pointer_cast<gl::Mesh>(theNode.shared_from_this()));
        }
        const std::list<gl::MeshPtr>& objects() const {return m_objects;}
        std::list<gl::MeshPtr>& objects() {return m_objects;}
        
    private:
        std::list<gl::MeshPtr> m_objects;
    };
}}

#endif /* defined(__gl__Mesh__) */
