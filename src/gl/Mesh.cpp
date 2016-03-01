// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Mesh.hpp"
#include "Visitor.hpp"

namespace kinski { namespace gl {

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

    Mesh::Mesh(const Geometry::Ptr &theGeom, const Material::Ptr &theMaterial):
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
        m_entries.push_back(entry);
    }

    Mesh::~Mesh()
    {
#ifndef KINSKI_NO_VAO
        GL_SUFFIX(glDeleteVertexArrays)(m_vertexArrays.size(), &m_vertexArrays[0]);
#endif
    }

    void Mesh::create_vertex_attribs()
    {
        m_vertex_attribs.clear();
        m_geometry->createGLBuffers();

        VertexAttrib vertices(m_vertexLocationName, m_geometry->vertexBuffer());
        m_vertex_attribs.push_back(vertices);

        if(m_geometry->hasTexCoords())
        {
            VertexAttrib tex_coords(m_texCoordLocationName, m_geometry->texCoordBuffer());
            tex_coords.size = 2;
            m_vertex_attribs.push_back(tex_coords);
        }

        if(m_geometry->hasColors())
        {
            VertexAttrib colors(m_colorLocationName, m_geometry->colorBuffer());
            colors.size = 4;
            m_vertex_attribs.push_back(colors);
        }

        if(m_geometry->hasNormals())
        {
            VertexAttrib normals(m_normalLocationName, m_geometry->normalBuffer());
            normals.size = 3;
            m_vertex_attribs.push_back(normals);
        }

        if(m_geometry->hasTangents())
        {
            VertexAttrib tangents(m_tangentLocationName, m_geometry->tangentBuffer());
            tangents.size = 3;
            m_vertex_attribs.push_back(tangents);
        }

        if(m_geometry->hasPointSizes())
        {
            VertexAttrib point_sizes(m_pointSizeLocationName, m_geometry->pointSizeBuffer());
            point_sizes.size = 1;
            m_vertex_attribs.push_back(point_sizes);
        }

        if(m_geometry->hasBones())
        {
            // bone IDs
            VertexAttrib bone_IDs(m_boneIDsLocationName, m_geometry->boneBuffer());
            bone_IDs.size = 4;
#if !defined(KINSKI_GLES)
            bone_IDs.type = GL_INT;
#else
            bone_IDs.type = GL_FLOAT;
#endif
            m_vertex_attribs.push_back(bone_IDs);

            // bone weights
            VertexAttrib bone_weights(m_boneWeightsLocationName, m_geometry->boneBuffer());
            bone_weights.size = 4;
            bone_weights.type = GL_FLOAT;
            bone_weights.offset = sizeof(glm::ivec4);
            m_vertex_attribs.push_back(bone_weights);
        }
    }

    void Mesh::bindVertexPointers(int material_index)
    {
        if(m_vertex_attribs.empty()){ create_vertex_attribs(); }

        Shader& shader = m_materials[material_index]->shader();
        if(!shader)
            throw Exception("No Shader defined in Mesh::createVertexArray()");

        // shader.bind();

        for(auto &vertex_attrib : m_vertex_attribs)
        {
            GLint location = shader.getAttribLocation(vertex_attrib.name);

            if(location >= 0)
            {
                vertex_attrib.buffer.bind();
                glEnableVertexAttribArray(location);

#if !defined(KINSKI_GLES)
                if(vertex_attrib.type == GL_FLOAT)
                {
                    glVertexAttribPointer(location, vertex_attrib.size, vertex_attrib.type,
                                          vertex_attrib.normalize, vertex_attrib.buffer.stride(),
                                          BUFFER_OFFSET(vertex_attrib.offset));
                }
                else if(vertex_attrib.type == GL_INT)
                {
                    glVertexAttribIPointer(location, vertex_attrib.size, vertex_attrib.type,
                                           vertex_attrib.buffer.stride(),
                                           BUFFER_OFFSET(vertex_attrib.offset));
                }
#else
                glVertexAttribPointer(location, vertex_attrib.size, vertex_attrib.type,
                                      vertex_attrib.normalize, vertex_attrib.buffer.stride(),
                                      BUFFER_OFFSET(vertex_attrib.offset));
#endif
                KINSKI_CHECK_GL_ERRORS();
            }
        }

        // set standard values for some attribs, in case they're not defined
        if(!m_geometry->hasColors())
        {
            GLint colorAttribLocation = shader.getAttribLocation(m_colorLocationName);
            if(colorAttribLocation >= 0)
            {
                glVertexAttrib4f(colorAttribLocation, 1.0f, 1.0f, 1.0f, 1.0f);
            }
        }
        // if(!m_geometry->hasTexCoords())
        // {
        //     GLint texCoordLocation = shader.getAttribLocation(m_texCoordLocationName);
        //     if(texCoordLocation >= 0)
        //     {
        //         glVertexAttrib2f(texCoordLocation, 0.f, 0.f);
        //     }
        // }
        if(!m_geometry->hasPointSizes())
        {
            GLint pointSizeAttribLocation = shader.getAttribLocation(m_pointSizeLocationName);
            if(pointSizeAttribLocation >= 0)
            {
                glVertexAttrib1f(pointSizeAttribLocation, 1.0f);
            }
        }

        // index buffer
        if(m_geometry->hasIndices())
            m_geometry->indexBuffer().bind();
    }

    void Mesh::update(float time_delta)
    {
        Object3D::update(time_delta);

        if(m_rootBone && m_animation_index < m_animations.size())
        {
            auto &anim = m_animations[m_animation_index];
            anim.current_time = fmodf(anim.current_time + time_delta * anim.ticksPerSec * m_animation_speed,
                                      anim.duration);
            anim.current_time += anim.current_time < 0.f ? anim.duration : 0.f;

            m_boneMatrices.resize(get_num_bones(m_rootBone));
            buildBoneMatrices(anim.current_time, m_rootBone, glm::mat4(), m_boneMatrices);
        }
    }

    uint32_t Mesh::get_num_bones(const BonePtr &theRoot)
    {
        if(!theRoot){ return 0; }

        uint32_t ret = 1;
        std::list<BonePtr>::const_iterator it = theRoot->children.begin();
        for (; it != theRoot->children.end(); ++it){ ret += get_num_bones(*it); }
        return ret;
    }

    void Mesh::initBoneMatrices()
    {
        m_boneMatrices.resize(get_num_bones(m_rootBone));
        buildBoneMatrices(0, m_rootBone, glm::mat4(), m_boneMatrices);
    }

    void Mesh::buildBoneMatrices(float time, BonePtr bone, glm::mat4 parentTransform,
                                 std::vector<glm::mat4> &matrices)
    {
        if(m_animations.empty()) return;

        auto &anim = m_animations[m_animation_index];
        glm::mat4 boneTransform = bone->transform;

        const AnimationKeys &bonekeys = anim.boneKeys[bone];
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
        if(boneHasKeys)
            boneTransform = translation * rotation * scaleMatrix;

        bone->worldtransform = parentTransform * boneTransform;

        // add final transform
        matrices[bone->index] = bone->worldtransform * bone->offset;

        // recursion through all children
        std::list<BonePtr>::iterator it = bone->children.begin();
        for (; it != bone->children.end(); ++it)
        {
            buildBoneMatrices(time, *it, bone->worldtransform, matrices);
        }
    }

    AABB Mesh::boundingBox() const
    {
        AABB ret = m_geometry->boundingBox();
        mat4 global_trans = global_transform();
        ret.transform(global_trans);

        for (auto &c :children())
        {
            ret += c->boundingBox();
        }
        return ret;
    }

    gl::OBB Mesh::obb() const
    {
        gl::OBB ret(m_geometry->boundingBox(), global_transform());
        return ret;
    }

    void Mesh::createVertexArray()
    {
        if(m_geometry->vertices().empty()) return;

#ifndef KINSKI_NO_VAO
        GL_SUFFIX(glBindVertexArray)(0);
#endif
        create_vertex_attribs();

#ifndef KINSKI_NO_VAO
        m_shaders.clear();
        m_shaders.resize(m_materials.size());
        GL_SUFFIX(glDeleteVertexArrays)(m_vertexArrays.size(), &m_vertexArrays[0]);
        m_vertexArrays.clear();
        m_vertexArrays.resize(m_materials.size(), 0);
        GL_SUFFIX(glGenVertexArrays)(m_vertexArrays.size(), &m_vertexArrays[0]);

        for (uint32_t i = 0; i < m_vertexArrays.size(); i++)
        {
            GL_SUFFIX(glBindVertexArray)(m_vertexArrays[i]);
            bindVertexPointers(i);
            m_shaders[i] = m_materials[i]->shader();
        }
        GL_SUFFIX(glBindVertexArray)(0);
#endif
    }

    GLuint Mesh::vertexArray(uint32_t i) const
    {
        if(m_shaders[i] != m_materials[i]->shader())
        {
            throw WrongVertexArrayDefinedException(get_id());
        }
        return m_vertexArrays[i];
    };

    void Mesh::bind_vertex_array(uint32_t i)
    {
#if !defined(KINSKI_NO_VAO)

        if(i >= m_vertexArrays.size()){createVertexArray();}

        try{GL_SUFFIX(glBindVertexArray)(vertexArray(i));}
        catch(const WrongVertexArrayDefinedException &e)
        {
            createVertexArray();
            try{GL_SUFFIX(glBindVertexArray)(m_vertexArrays[i]);}
            catch(std::exception &e)
            {
                // should not arrive here
                LOG_ERROR<<e.what();
                return;
            }
        }
#endif
    }

    MeshPtr Mesh::copy()
    {
        MeshPtr ret = create(m_geometry, material());
        *ret = *this;

        // deep copy bones
//        ret->rootBone() = deep_copy_bones(rootBone());
//
//        // remap animations
//        std::vector<MeshAnimation> anim_cp;
//
//        for(const auto &anim : m_animations)
//        {
//            auto cp = anim;
//            cp.boneKeys.clear();
//
//            for(const auto &bone_key : anim.boneKeys)
//            {
//                cp.boneKeys[get_bone_by_name(ret->rootBone(), bone_key.first->name)] = bone_key.second;
//            }
//            anim_cp.push_back(anim);
//        }
//        ret->m_animations = anim_cp;

        ret->m_vertexArrays.clear();
        ret->m_shaders.clear();
        ret->createVertexArray();
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

    void Mesh::setVertexLocationName(const std::string &theName)
    {
        m_vertexLocationName = theName;
        createVertexArray();
    }

    void Mesh::setNormalLocationName(const std::string &theName)
    {
        m_normalLocationName = theName;
        createVertexArray();
    }

    void Mesh::setTangentLocationName(const std::string &theName)
    {
        m_tangentLocationName = theName;
        createVertexArray();
    }

    void Mesh::setPointSizeLocationName(const std::string &theName)
    {
        m_pointSizeLocationName = theName;
        createVertexArray();
    }

    void Mesh::setTexCoordLocationName(const std::string &theName)
    {
        m_texCoordLocationName = theName;
        createVertexArray();
    }

    void Mesh::accept(Visitor &theVisitor)
    {
        theVisitor.visit(*this);
    }
}}
