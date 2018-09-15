// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Mesh.hpp"
#include "Visitor.hpp"

namespace kinski { namespace gl {

uint32_t num_bones_in_hierarchy(const BonePtr &the_root)
{
    if(!the_root){ return 0; }
    uint32_t ret = 1;
    for (const auto &b : the_root->children){ ret += num_bones_in_hierarchy(b); }
    return ret;
}

BonePtr deep_copy_bones(BonePtr src)
{
    if(!src){ return BonePtr(); }
    BonePtr ret = std::make_shared<Bone>();
    *ret = *src;
    ret->children.clear();

    for(const auto &c : src->children)
    {
        auto b = deep_copy_bones(c);
        b->parent = ret;
        ret->children.push_back(b);
    }
    return ret;
}

BonePtr get_bone_by_name(BonePtr root, const std::string &the_name)
{
    if(root->name == the_name){ return root; }
    for(const auto &c : root->children)
    {
        auto b = get_bone_by_name(c, the_name);
        if(b){ return b; }
    }
    return BonePtr();
}
    
Mesh::Mesh(const GeometryPtr &theGeom, const MaterialPtr &theMaterial):
Object3D(),
m_geometry(theGeom),
m_animation_index(0),
m_animation_speed(1.f),
m_vertexLocationName("a_vertex"),
m_normalLocationName("a_normal"),
m_tangentLocationName("a_tangent"),
m_pointSizeLocationName("a_pointSize"),
m_texCoordLocationName("a_texCoord"),
m_colorLocationName("a_color"),
m_boneIDsLocationName("a_boneIds"),
m_boneWeightsLocationName("a_boneWeights")
{
    m_materials.push_back(theMaterial);
    Entry entry;
    entry.num_vertices = theGeom->vertices().size();
    entry.num_indices = theGeom->indices().size();
    entry.base_index = entry.base_vertex = 0;
    entry.material_index = 0;
    entry.primitive_type = theGeom->primitive_type();
    m_entries.push_back(entry);
}

Mesh::~Mesh()
{
    for(auto &mat : materials()){ gl::context()->clear_vao(geometry(), mat->shader()); }
}

void Mesh::create_vertex_attribs(bool recreate)
{
    if(!m_vertex_attribs.empty() && !recreate) return;

    m_vertex_attribs.clear();
    m_geometry->create_gl_buffers();

    VertexAttrib vertices;
    vertices.name = m_vertexLocationName;
    vertices.buffer = m_geometry->vertex_buffer();
    m_vertex_attribs.insert(std::make_pair(Geometry::VERTEX_BIT, vertices));

    if(m_geometry->has_tex_coords())
    {
        VertexAttrib tex_coords;
        tex_coords.name = m_texCoordLocationName;
        tex_coords.buffer = m_geometry->tex_coord_buffer();
        tex_coords.size = 2;
        m_vertex_attribs.insert(std::make_pair(Geometry::TEXCOORD_BIT, tex_coords));
    }

    if(m_geometry->has_colors())
    {
        VertexAttrib colors;
        colors.name = m_colorLocationName;
        colors.buffer = m_geometry->color_buffer();
        colors.size = 4;
        m_vertex_attribs.insert(std::make_pair(Geometry::COLOR_BIT, colors));
    }

    if(m_geometry->has_normals())
    {
        VertexAttrib normals;
        normals.name = m_normalLocationName;
        normals.buffer = m_geometry->normal_buffer();
        normals.size = 3;
        m_vertex_attribs.insert(std::make_pair(Geometry::NORMAL_BIT, normals));
    }

    if(m_geometry->has_tangents())
    {
        VertexAttrib tangents;
        tangents.name = m_tangentLocationName;
        tangents.buffer = m_geometry->tangent_buffer();
        tangents.size = 3;
        m_vertex_attribs.insert(std::make_pair(Geometry::TANGENT_BIT, tangents));
    }

    if(m_geometry->has_point_sizes())
    {
        VertexAttrib point_sizes;
        point_sizes.name = m_pointSizeLocationName;
        point_sizes.buffer = m_geometry->point_size_buffer();
        point_sizes.size = 1;
        m_vertex_attribs.insert(std::make_pair(Geometry::POINTSIZE_BIT, point_sizes));
    }

    if(m_geometry->has_bones())
    {
        // bone IDs
        VertexAttrib bone_indices;
        bone_indices.name = m_boneIDsLocationName;
        bone_indices.buffer = m_geometry->bone_buffer();
        bone_indices.size = 4;
#if !defined(KINSKI_GLES_2)
        bone_indices.type = GL_INT;
#else
        bone_indices.type = GL_FLOAT;
#endif
        m_vertex_attribs.insert(std::make_pair(Geometry::BONE_INDEX_BIT, bone_indices));

        // bone weights
        VertexAttrib bone_weights;
        bone_weights.name = m_boneWeightsLocationName;
        bone_weights.buffer = m_geometry->bone_buffer();
        bone_weights.size = 4;
        bone_weights.type = GL_FLOAT;
        bone_weights.offset = offsetof(BoneVertexData, weights);
        m_vertex_attribs.insert(std::make_pair(Geometry::BONE_WEIGHT_BIT, bone_weights));
    }
}

void Mesh::add_vertex_attrib(const VertexAttrib& v)
{
    m_vertex_attribs.insert(std::make_pair(Geometry::CUSTOM_BIT, v));
}
    
void Mesh::bind_vertex_pointers(const gl::ShaderPtr &the_shader)
{
    if(m_vertex_attribs.empty()){ create_vertex_attribs(); }

    the_shader->bind();

    for(auto &p : m_vertex_attribs)
    {
        const auto &vertex_attrib = p.second;

        // determine glsl location for attrib
        int32_t location = -1;
        if(vertex_attrib.location >= 0){ location = vertex_attrib.location; }
        else{ location = the_shader->attrib_location(vertex_attrib.name); }
        KINSKI_CHECK_GL_ERRORS();
        
        if(location >= 0)
        {
            vertex_attrib.buffer.bind();
            glEnableVertexAttribArray(location);
            KINSKI_CHECK_GL_ERRORS();
            
            if(vertex_attrib.type == GL_FLOAT || vertex_attrib.type == GL_UNSIGNED_BYTE)
            {
                glVertexAttribPointer(location, vertex_attrib.size, vertex_attrib.type,
                                      vertex_attrib.normalize, vertex_attrib.buffer.stride(),
                                      BUFFER_OFFSET(vertex_attrib.offset));
            }
#if !defined(KINSKI_GLES_2)
            else if(vertex_attrib.type == GL_INT)
            {
                glVertexAttribIPointer(location, vertex_attrib.size, vertex_attrib.type,
                                       vertex_attrib.buffer.stride(),
                                       BUFFER_OFFSET(vertex_attrib.offset));
            }
#endif
            KINSKI_CHECK_GL_ERRORS();
        }
    }
    
    // bind index buffer
    if(m_index_buffer){ m_index_buffer.bind(); }
    else if(m_geometry->has_indices()){ m_geometry->index_buffer().bind(); }
}

void Mesh::bind_vertex_pointers(int material_index)
{
    ShaderPtr shader = m_materials[material_index]->shader();
    bind_vertex_pointers(shader);
}

void Mesh::update(float time_delta)
{
    Object3D::update(time_delta);

    if(m_rootBone && m_animation_index < m_animations.size())
    {
        auto &anim = m_animations[m_animation_index];
        anim.current_time = fmodf(anim.current_time + time_delta * anim.ticks_per_sec * m_animation_speed,
                                  anim.duration);
        anim.current_time += anim.current_time < 0.f ? anim.duration : 0.f;

        m_boneMatrices.resize(num_bones());
        build_bone_matrices(m_rootBone, m_boneMatrices);
    }
}

uint32_t Mesh::num_bones()
{
    return num_bones_in_hierarchy(root_bone());
}

void Mesh::build_bone_matrices(BonePtr bone, std::vector<glm::mat4> &matrices,
                               glm::mat4 parentTransform)
{
    if(m_animations.empty()) return;

    auto &anim = m_animations[m_animation_index];
    float time = anim.current_time;
    glm::mat4 boneTransform = bone->transform;

    const AnimationKeys &bonekeys = anim.bone_keys[bone];
    bool boneHasKeys = false;

    // translation
    glm::mat4 translation;
    if(!bonekeys.positionkeys.empty())
    {
        boneHasKeys = true;
        uint32_t i = 0;
        for (; i < bonekeys.positionkeys.size() - 1; i++)
        {
            const Key<glm::vec3> &key = bonekeys.positionkeys[i + 1];
            if(key.time >= time)
                break;
        }
        // i now holds the correct time index
        const Key<glm::vec3> &key1 = bonekeys.positionkeys[i],
                key2 = bonekeys.positionkeys[(i + 1) % bonekeys.positionkeys.size()];

        float startTime = key1.time;
        float endTime = key2.time < key1.time ? key2.time + anim.duration : key2.time;
        float frac = std::max( (time - startTime) / (endTime - startTime), 0.0f);
        glm::vec3 pos = glm::mix(key1.value, key2.value, frac);
        translation = glm::translate(translation, pos);
    }

    // rotation
    glm::mat4 rotation;
    if(!bonekeys.rotationkeys.empty())
    {
        boneHasKeys = true;
        uint32_t i = 0;
        for (; i < bonekeys.rotationkeys.size() - 1; i++)
        {
            const Key<glm::quat> &key = bonekeys.rotationkeys[i+1];
            if(key.time >= time)
                break;
        }
        // i now holds the correct time index
        const Key<glm::quat> &key1 = bonekeys.rotationkeys[i],
                key2 = bonekeys.rotationkeys[(i + 1) % bonekeys.rotationkeys.size()];

        float startTime = key1.time;
        float endTime = key2.time < key1.time ? key2.time + anim.duration : key2.time;
        float frac = std::max( (time - startTime) / (endTime - startTime), 0.0f);

        // quaternion spherical linear interpolation
        glm::quat interpolRot = glm::slerp(key1.value, key2.value, frac);
        rotation = glm::mat4_cast(interpolRot);
    }

    // scale
    glm::mat4 scaleMatrix;
    if(!bonekeys.scalekeys.empty())
    {
        if(bonekeys.scalekeys.size() == 1)
        {
            scaleMatrix = glm::scale(scaleMatrix, bonekeys.scalekeys.front().value);
        }
        else
        {
            boneHasKeys = true;
            uint32_t i = 0;
            for (; i < bonekeys.scalekeys.size() - 1; i++)
            {
                const Key<glm::vec3> &key = bonekeys.scalekeys[i + 1];
                if(key.time >= time)
                    break;
            }
            // i now holds the correct time index
            const Key<glm::vec3> &key1 = bonekeys.scalekeys[i],
                    key2 = bonekeys.scalekeys[(i + 1) % bonekeys.scalekeys.size()];

            float startTime = key1.time;
            float endTime = key2.time < key1.time ? key2.time + anim.duration : key2.time;
            float frac = std::max( (time - startTime) / (endTime - startTime), 0.0f);
            glm::vec3 scale = glm::mix(key1.value, key2.value, frac);
            scaleMatrix = glm::scale(scaleMatrix, scale);
        }
    }
    if(boneHasKeys){ boneTransform = translation * rotation * scaleMatrix; }
    bone->worldtransform = parentTransform * boneTransform;

    // add final transform
    matrices[bone->index] = bone->worldtransform * bone->offset;

    // recursion through all children
    for(auto &b : bone->children){ build_bone_matrices(b, matrices, bone->worldtransform); }
}

AABB Mesh::aabb() const
{
    AABB ret = m_geometry->aabb();
    mat4 global_trans = global_transform();
    ret.transform(global_trans);
    for(auto &c :children()){ ret += c->aabb(); }
    return ret;
}

gl::OBB Mesh::obb() const
{
    gl::OBB ret(m_geometry->aabb(), global_transform());
    return ret;
}

GLuint Mesh::create_vertex_array(const gl::ShaderPtr &the_shader)
{
    GLuint vao_id = 0;

#ifndef KINSKI_NO_VAO
    GL_SUFFIX(glBindVertexArray)(0);
#endif
    create_vertex_attribs();

#ifndef KINSKI_NO_VAO
    vao_id = gl::context()->get_vao(m_geometry, the_shader);
    vao_id = vao_id ? vao_id : gl::context()->create_vao(m_geometry, the_shader);
    GL_SUFFIX(glBindVertexArray)(vao_id);
    bind_vertex_pointers(the_shader);
    GL_SUFFIX(glBindVertexArray)(0);
#endif
    return vao_id;
}

GLuint Mesh::vertex_array(uint32_t i) const
{
    return vertex_array(m_materials[i]->shader());
};

GLuint Mesh::vertex_array(const gl::ShaderPtr &the_shader) const
{
    return gl::context()->get_vao(m_geometry, the_shader);
}

void Mesh::bind_vertex_array(uint32_t i)
{
    bind_vertex_array(m_materials[i]->shader());
}

void Mesh::bind_vertex_array(const gl::ShaderPtr &the_shader)
{
#if !defined(KINSKI_NO_VAO)
    GLuint vao_id = vertex_array(the_shader);
    if(!vao_id){ vao_id = create_vertex_array(the_shader); }
    GL_SUFFIX(glBindVertexArray)(vao_id);
#else
    bind_vertex_pointers(the_shader);
#endif
}

MeshPtr Mesh::copy()
{
    MeshPtr ret = create(m_geometry, material());
    *ret = *this;

    //TODO: deep copy bones, rebuild animations

    return ret;
}

void Mesh::set_animation_index(uint32_t the_index)
{
    m_animation_index = clamp<uint32_t>(the_index, 0, m_animations.size() - 1);
    if(!m_animations.empty() && the_index != m_animation_index)
    {
        m_animations[m_animation_index].current_time = 0.f;
    }
}
    
void Mesh::accept(Visitor &theVisitor)
{
    theVisitor.visit(*this);
}
}}
