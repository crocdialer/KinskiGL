// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Mesh.h"
#include "Visitor.h"

namespace kinski { namespace gl {
  
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
        for (uint32_t i = 0; i < m_vertexArrays.size(); i++)
        {
            if(m_vertexArrays[i]) GL_SUFFIX(glDeleteVertexArrays)(1, &m_vertexArrays[i]);
        }
#endif
    }
    
    void Mesh::bindVertexPointers(int material_index) const
    {
        Shader& shader = m_materials[material_index]->shader();
        if(!shader)
            throw Exception("No Shader defined in Mesh::createVertexArray()");
        
        // bind our shader, not sure if necessary here
        //        shader.bind();
        
        // create VBOs if not yet existing
        if(!m_geometry->vertexBuffer())
            m_geometry->createGLBuffers();
        
        // define attrib pointer (vertex)
        GLuint vertexAttribLocation = shader.getAttribLocation(m_vertexLocationName);
        m_geometry->vertexBuffer().bind();
        glEnableVertexAttribArray(vertexAttribLocation);
        glVertexAttribPointer(vertexAttribLocation, 3, GL_FLOAT, GL_FALSE,
                              m_geometry->vertexBuffer().stride(), BUFFER_OFFSET(0));
        KINSKI_CHECK_GL_ERRORS();
        
        if(m_geometry->hasTexCoords())
        {
            GLint texCoordAttribLocation = shader.getAttribLocation(m_texCoordLocationName);
            
            if(texCoordAttribLocation >= 0)
            {
                m_geometry->texCoordBuffer().bind();
                // define attrib pointer (texcoord)
                glEnableVertexAttribArray(texCoordAttribLocation);
                glVertexAttribPointer(texCoordAttribLocation, 2, GL_FLOAT, GL_FALSE,
                                      m_geometry->texCoordBuffer().stride(), BUFFER_OFFSET(0));
                KINSKI_CHECK_GL_ERRORS();
            }
        }
        
        if(m_geometry->hasColors())
        {
            GLint colorAttribLocation = shader.getAttribLocation(m_colorLocationName);
            
            if(colorAttribLocation >= 0)
            {
                m_geometry->colorBuffer().bind();
                // define attrib pointer (colors)
                glEnableVertexAttribArray(colorAttribLocation);
                glVertexAttribPointer(colorAttribLocation, 4, GL_FLOAT, GL_FALSE,
                                      m_geometry->colorBuffer().stride(), BUFFER_OFFSET(0));
                KINSKI_CHECK_GL_ERRORS();
            }
        }else{
            GLint colorAttribLocation = shader.getAttribLocation(m_colorLocationName);
            if(colorAttribLocation >= 0) glVertexAttrib4f(colorAttribLocation, 1.0f, 1.0f, 1.0f, 1.0f);
        }
        
        if(m_geometry->hasNormals())
        {
            GLint normalAttribLocation = shader.getAttribLocation(m_normalLocationName);
            
            if(normalAttribLocation >= 0)
            {
                m_geometry->normalBuffer().bind();
                // define attrib pointer (normal)
                glEnableVertexAttribArray(normalAttribLocation);
                glVertexAttribPointer(normalAttribLocation, 3, GL_FLOAT, GL_FALSE,
                                      m_geometry->normalBuffer().stride(), BUFFER_OFFSET(0));
                KINSKI_CHECK_GL_ERRORS();
            }
        }
        
        if(m_geometry->hasTangents())
        {
            GLint tangentAttribLocation = shader.getAttribLocation(m_tangentLocationName);
            
            if(tangentAttribLocation >= 0)
            {
                m_geometry->tangentBuffer().bind();
                // define attrib pointer (tangent)
                glEnableVertexAttribArray(tangentAttribLocation);
                glVertexAttribPointer(tangentAttribLocation, 3, GL_FLOAT, GL_FALSE,
                                      m_geometry->tangentBuffer().stride(), BUFFER_OFFSET(0));
                KINSKI_CHECK_GL_ERRORS();
            }
        }
        
        if(m_geometry->hasPointSizes())
        {
            GLint pointSizeAttribLocation = shader.getAttribLocation(m_pointSizeLocationName);
            
            if(pointSizeAttribLocation >= 0)
            {
                m_geometry->pointSizeBuffer().bind();
                // define attrib pointer (pointsize)
                glEnableVertexAttribArray(pointSizeAttribLocation);
                glVertexAttribPointer(pointSizeAttribLocation, 1, GL_FLOAT, GL_FALSE,
                                      m_geometry->pointSizeBuffer().stride(), BUFFER_OFFSET(0));
                KINSKI_CHECK_GL_ERRORS();
            }
        }else{
            GLint pointSizeAttribLocation = shader.getAttribLocation(m_pointSizeLocationName);
            if(pointSizeAttribLocation >= 0) glVertexAttrib1f(pointSizeAttribLocation, 1.0f);
        }

        if(m_geometry->hasBones())
        {
            GLint boneIdsAttribLocation = shader.getAttribLocation(m_boneIDsLocationName);
            GLint boneWeightsAttribLocation = shader.getAttribLocation(m_boneWeightsLocationName);
            
            if(boneIdsAttribLocation >= 0 && boneWeightsAttribLocation >= 0)
            {
                m_geometry->boneBuffer().bind();
                // define attrib pointer (boneIDs)
                glEnableVertexAttribArray(boneIdsAttribLocation);
                
                // use ivec4 to submit bone-indices on Dekstop GL
#ifndef KINSKI_GLES
                glVertexAttribIPointer(boneIdsAttribLocation, 4, GL_INT,
                                       m_geometry->boneBuffer().stride(), BUFFER_OFFSET(0));
                
                // else fall back to float vec4 for GLES2
#else
                glVertexAttribPointer(boneIdsAttribLocation, 4, GL_INT, GL_FALSE,
                                      m_geometry->boneBuffer().stride(), BUFFER_OFFSET(0));
#endif
                KINSKI_CHECK_GL_ERRORS();
                
                // define attrib pointer (boneWeights)
                glEnableVertexAttribArray(boneWeightsAttribLocation);
                glVertexAttribPointer(boneWeightsAttribLocation, 4, GL_FLOAT, GL_FALSE,
                                      m_geometry->boneBuffer().stride(),
                                      BUFFER_OFFSET(sizeof(glm::ivec4)));

                KINSKI_CHECK_GL_ERRORS();
            }
        }
        
        // index buffer
        if(m_geometry->hasIndices())
            m_geometry->indexBuffer().bind();
    }
    
    void Mesh::update(float time_delta)
    {
        Object3D::update(time_delta);
        
        if(m_animation_index < m_animations.size())
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
        uint32_t ret = 1;
        std::list<BonePtr>::const_iterator it = theRoot->children.begin();
        for (; it != theRoot->children.end(); ++it)
        {
            ret += get_num_bones(*it);
        }
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
            
            // quaternion interpolation produces glitches
            //            glm::quat interpolRot = glm::mix(key1.value, key2.value, frac);
            //            rotation = glm::mat4_cast(interpolRot);
            glm::mat4 rot1 = glm::mat4_cast(key1.value), rot2 = glm::mat4_cast(key2.value);
            rotation = rot1 + frac * (rot2 - rot1);
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
        auto ret = m_geometry->boundingBox() + Object3D::boundingBox();
        return ret;
    }
    
    void Mesh::createVertexArray()
    {
        if(m_geometry->vertices().empty()) return;
        
#ifndef KINSKI_NO_VAO
        for (uint32_t i = 0; i < m_vertexArrays.size(); i++)
        {
            if(m_vertexArrays[i]) GL_SUFFIX(glDeleteVertexArrays)(1, &m_vertexArrays[i]);
        }
        m_vertexArrays.clear();
        m_shaders.clear();
        m_vertexArrays.resize(m_materials.size(), 0);
        m_shaders.resize(m_materials.size());
        
        for (uint32_t i = 0; i < m_vertexArrays.size(); i++)
        {
            if(!m_vertexArrays[i]){ GL_SUFFIX(glGenVertexArrays)(1, &m_vertexArrays[i]); }
            
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
            throw WrongVertexArrayDefinedException(getID());
        }
        return m_vertexArrays[i];
    };
    
    void Mesh::bind_vertex_array(uint32_t i)
    {
#if !defined(KINSKI_NO_VAO)
        i = std::max<uint32_t>(0, i);
        
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
        MeshPtr ret = create(m_geometry, m_materials[0]);
        *ret = *this;
        ret->m_vertexArrays.clear();
        ret->m_shaders.clear();
        ret->createVertexArray();
        return ret;
    }
    
    void Mesh::set_animation_index(uint32_t the_index)
    {
        m_animation_index = clamp<uint32_t>(the_index, 0, m_animations.size() - 1);
        if(!m_animations.empty()){ m_animations[m_animation_index].current_time = 0.f; }
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
