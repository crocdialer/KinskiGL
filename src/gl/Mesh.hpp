// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "Object3D.hpp"
#include "Geometry.hpp"
#include "Material.hpp"

namespace kinski { namespace gl {
    
    class Bone
    {
    public:
        std::string name;
        mat4 transform;
        mat4 worldtransform;
        mat4 offset;
        uint32_t index;
        BonePtr parent;
        std::list<BonePtr> children;
    };
    
    BonePtr deep_copy_bones(BonePtr src);
    BonePtr get_bone_by_name(BonePtr root, const std::string &the_name);
    
    template<typename T> struct Key
    {
        float time;
        T value;
        
        Key(float t, const T &v):time(t), value(v){};
    };
    
    struct AnimationKeys
    {
        std::vector< Key<vec3> > positionkeys;
        std::vector< Key<quat> > rotationkeys;
        std::vector< Key<vec3> > scalekeys;
    };
    
    class MeshAnimation
    {
    public:
        float current_time;
        float duration;
        float ticksPerSec;
        std::map<BonePtr, AnimationKeys> boneKeys;
        MeshAnimation():current_time(0), ticksPerSec(1.0f){};
    };
    
    class KINSKI_API Mesh : public Object3D
    {
    public:
        
        struct Entry
        {
            Entry():num_indices(0), num_vertices(0), base_vertex(0), base_index(0),
            material_index(0), primitive_type(0), enabled(true){}
            
            uint32_t num_indices, num_vertices;
            uint32_t base_vertex, base_index;
            uint32_t material_index;
            uint32_t primitive_type;
            bool enabled;
        };
        
        struct VertexAttrib
        {
            VertexAttrib(const std::string &the_name, const gl::Buffer &the_buffer):
            name(the_name), buffer(the_buffer){}
            std::string name;
            gl::Buffer buffer;
            size_t size = 3;
            size_t offset = 0;
            GLenum type = GL_FLOAT;
            bool normalize = false;
        };
        
        typedef std::shared_ptr<Mesh> Ptr;
        typedef std::shared_ptr<const Mesh> ConstPtr;
        
        static Ptr create(const GeometryPtr &theGeom = gl::Geometry::create(),
                          const MaterialPtr &theMaterial = gl::Material::create())
        {
            return Ptr(new Mesh(theGeom, theMaterial));
        }

        virtual ~Mesh();
        
        const GeometryPtr& geometry() const { return m_geometry; };
        GeometryPtr& geometry() { return m_geometry; };
        
        const MaterialPtr& material() const { return m_materials.front(); };
        MaterialPtr& material() { return m_materials.front(); };
        
        //! create the vertex attrib structures corresponding to the attached Geometry
        // and store them in <m_vertex_attribs>
        void create_vertex_attribs();
        
        void bindVertexPointers(int material_index = 0);
        void bind_vertex_array(uint32_t i = 0);
        void createVertexArray();
        GLuint vertexArray(uint32_t i = 0) const;
        
        void update(float time_delta) override;
        
        /*!
         * returns an AABB with global transform applied to it
         */
        AABB bounding_box() const override;
        
        gl::OBB obb() const override;
        
        const std::vector<Entry>& entries() const {return m_entries;};
        std::vector<Entry>& entries() {return m_entries;};
        
        const std::vector<MaterialPtr>& materials() const {return m_materials;};
        std::vector<MaterialPtr>& materials() {return m_materials;};
        
        const std::vector<MeshAnimation>& animations() const { return m_animations; };
        std::vector<MeshAnimation>& animations() { return m_animations; };
        void addAnimation(const MeshAnimation &theAnim) { m_animations.push_back(theAnim); };
        
        uint32_t animation_index() const { return m_animation_index; }
        void set_animation_index(uint32_t the_index);
        
        float animation_speed() const { return m_animation_speed; }
        void set_animation_speed(const float the_speed) { m_animation_speed = the_speed; }
        
        std::vector<mat4>& boneMatrices(){ return m_boneMatrices; };
        const std::vector<mat4>& boneMatrices() const { return m_boneMatrices; };
        
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
        
        /*!
         *  return a copy of this mesh, sharing its geometry, materials, animations, etc.
         */
        MeshPtr copy();
        
        virtual void accept(Visitor &theVisitor) override;
        
    private:
        
        Mesh(const Geometry::Ptr &theGeom, const MaterialPtr &theMaterial);
        
        void buildBoneMatrices(float time, BonePtr bone,
                               mat4 parentTransform,
                               std::vector<mat4> &matrices);
        
        GeometryPtr m_geometry;
        std::vector<Entry> m_entries;
        std::vector<MaterialPtr> m_materials;
        std::vector<GLuint> m_vertexArrays;
        
        std::vector<gl::Shader> m_shaders;
        
        // skeletal animations stuff
        BonePtr m_rootBone;
        uint32_t m_animation_index;
        std::vector<MeshAnimation> m_animations;
        float m_animation_speed;
        std::vector<mat4> m_boneMatrices;
        
        //! holds our vertex attributes
        std::vector<VertexAttrib> m_vertex_attribs;
        
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
        Exception("wrong vertex array defined for object: " + kinski::to_string(theID)){}
    };    
}}
