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

    template<typename T> struct Key
    {
        float time;
        T value;
        
        Key(float t, const T &v):time(t), value(v){};
    };
    
    struct AnimationKeys
    {
        std::vector<Key<vec3>> positionkeys;
        std::vector<Key<quat>> rotationkeys;
        std::vector<Key<vec3>> scalekeys;
    };
    
    class MeshAnimation
    {
    public:
        float current_time;
        float duration;
        float ticks_per_sec;
        std::map<BonePtr, AnimationKeys> bone_keys;
        MeshAnimation():current_time(0), ticks_per_sec(1.0f){};
    };
    
    BonePtr deep_copy_bones(BonePtr src);
    
    BonePtr get_bone_by_name(BonePtr root, const std::string &the_name);

    uint32_t num_bones_in_hierarchy(const BonePtr &the_root);

    class Mesh : public Object3D
    {
    public:
        
        struct Entry
        {
            uint32_t num_indices = 0;
            uint32_t num_vertices = 0;
            index_t base_vertex = 0;
            index_t base_index = 0;
            uint32_t material_index = 0;
            uint32_t primitive_type = 0;
            bool enabled = true;
        };
        
        struct VertexAttrib
        {
            std::string name;
            int32_t location = -1;
            gl::Buffer buffer;
            size_t size = 3;
            size_t offset = 0;
            size_t stride = 0;
            GLenum type = GL_FLOAT;
            uint32_t instance_rate = 0;
            bool normalize = false;
        };
        
        static MeshPtr create(const GeometryPtr &theGeom = gl::Geometry::create(),
                              const MaterialPtr &theMaterial = gl::Material::create())
        {
            return MeshPtr(new Mesh(theGeom, theMaterial));
        }

        virtual ~Mesh();
        
        const GeometryConstPtr geometry() const { return m_geometry; };
        GeometryPtr& geometry() { return m_geometry; };
        
        const MaterialPtr material() const;
        
        //! create the vertex attrib structures corresponding to the attached Geometry
        // and store them in <m_vertex_attribs>
        void create_vertex_attribs(bool recreate = false);
        
        //! add a custom VertexAttrib
        void add_vertex_attrib(const VertexAttrib& v);

        void set_index_buffer(const gl::Buffer &the_buffer){ m_index_buffer = the_buffer; };
        gl::Buffer index_buffer() const { return m_index_buffer; };
        
        void bind_vertex_pointers(int material_index = 0);
        void bind_vertex_pointers(const gl::ShaderPtr &the_shader);
        void bind_vertex_array(uint32_t i = 0);
        void bind_vertex_array(const gl::ShaderPtr &the_shader);
        uint32_t vertex_array(uint32_t i = 0) const;
        uint32_t vertex_array(const gl::ShaderPtr &the_shader) const;
        GLuint create_vertex_array(const gl::ShaderPtr &the_shader);

        void update(float time_delta) override;
        
        /*!
         * returns an AABB with global transform applied to it
         */
        AABB aabb() const override;
        
        gl::OBB obb() const override;
        
        const std::vector<Entry>& entries() const {return m_entries;};
        std::vector<Entry>& entries() {return m_entries;};
        
        const std::vector<MaterialPtr>& materials() const {return m_materials;};
        std::vector<MaterialPtr>& materials() {return m_materials;};
        
        const std::vector<MeshAnimation>& animations() const { return m_animations; };
        std::vector<MeshAnimation>& animations() { return m_animations; };
        void add_animation(const MeshAnimation &theAnim) { m_animations.push_back(theAnim); };
        
        uint32_t animation_index() const { return m_animation_index; }
        void set_animation_index(uint32_t the_index);
        
        float animation_speed() const { return m_animation_speed; }
        void set_animation_speed(const float the_speed) { m_animation_speed = the_speed; }

        const std::vector<mat4>& bone_matrices() const { return m_boneMatrices; };

        const BonePtr& root_bone() const { return m_rootBone; };
        void set_root_bone(BonePtr b){ m_rootBone = b; };

        uint32_t num_bones();
        
        /*!
         *  return a copy of this mesh, sharing its geometry, materials, animations, etc.
         */
        MeshPtr copy();
        
        virtual void accept(Visitor &theVisitor) override;
        
    private:
        
        Mesh(const GeometryPtr &theGeom, const MaterialPtr &theMaterial);

        void build_bone_matrices(BonePtr bone, std::vector<glm::mat4> &matrices,
                                 glm::mat4 parentTransform = glm::mat4());

        GeometryPtr m_geometry;
        std::vector<Entry> m_entries;
        std::vector<MaterialPtr> m_materials;

        //! optional index buffer, can be used to override the index buffer from m_geometry
        gl::Buffer m_index_buffer;

        // skeletal animations stuff
        BonePtr m_rootBone;
        uint32_t m_animation_index;
        std::vector<MeshAnimation> m_animations;
        float m_animation_speed;
        std::vector<mat4> m_boneMatrices;
        
        //! holds our vertex attributes, using gl::Geometry::BufferBit as keys
        std::unordered_multimap<uint32_t, VertexAttrib> m_vertex_attribs;
        
        std::string m_vertexLocationName;
        std::string m_normalLocationName;
        std::string m_tangentLocationName;
        std::string m_pointSizeLocationName;
        std::string m_texCoordLocationName;
        std::string m_colorLocationName;
        std::string m_boneIDsLocationName;
        std::string m_boneWeightsLocationName;
    };
}}
