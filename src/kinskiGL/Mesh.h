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
            Entry():numdices(0), base_vertex(0), base_index(0), material_index(-1){}
            uint32_t numdices;
            uint32_t base_vertex;
            uint32_t base_index;
            int material_index;
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
        
        const MaterialPtr& material() const { return m_material; };
        MaterialPtr& material() { return m_material; };
        
        void bindVertexPointers() const;
        void createVertexArray();
        GLuint vertexArray() const;
        
        void update(float time_delta);
        AABB boundingBox() const;
        
        const std::vector<Entry>& entries() const {return m_entries;};
        std::vector<Entry>& entries() {return m_entries;};
        
        const std::vector<MaterialPtr>& materials() const {return m_materials;};
        std::vector<MaterialPtr>& materials() {return m_materials;};
        
        const AnimationPtr animation() const { return m_animation; };
        AnimationPtr animation() { return m_animation; };
        void setAnimation(const std::shared_ptr<Animation> &theAnim) { m_animation = theAnim; };
        
        std::vector<glm::mat4>& boneMatrices(){ return m_boneMatrices; };
        const std::vector<glm::mat4>& boneMatrices() const { return m_boneMatrices; };
        
        BonePtr& rootBone(){ return m_rootBone; };
        const BonePtr& rootBone() const { return m_rootBone; };
        
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
         * Defaults to "a_texCoord"
         */
        void setTexCoordLocationName(const std::string &theName);
        
        /*!
         * Set the name under which the attribute will be accessible in the shader.
         * Defaults to "a_color"
         */
        void setColorLocationName(const std::string &theName);
        
    private:
        
        Mesh(const Geometry::Ptr &theGeom, const Material::Ptr &theMaterial);
        
        void buildBoneMatrices(float time, BonePtr bone,
                               glm::mat4 parentTransform,
                               std::vector<glm::mat4> &matrices);
        
        GeometryPtr m_geometry;
        MaterialPtr m_material;
        
        std::vector<Entry> m_entries;
        std::vector<MaterialPtr> m_materials;
        
        GLuint m_vertexArray;
        mutable std::pair<MaterialPtr, GLuint> m_material_vertex_array_mapping;
        
        // skeletal animations stuff
        AnimationPtr m_animation;
        BonePtr m_rootBone;
        std::vector<glm::mat4> m_boneMatrices;
        
        std::string m_vertexLocationName;
        std::string m_normalLocationName;
        std::string m_tangentLocationName;
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
}}

#endif /* defined(__kinskiGL__Mesh__) */
