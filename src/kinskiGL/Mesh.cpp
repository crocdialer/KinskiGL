// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Mesh.h"

namespace kinski { namespace gl {
  
    Mesh::Mesh(const Geometry::Ptr &theGeom, const Material::Ptr &theMaterial):
    Object3D(),
    m_geometry(theGeom),
    m_material(theMaterial),
    m_vertexArray(0),
    m_vertexLocationName("a_vertex"),
    m_normalLocationName("a_normal"),
    m_tangentLocationName("a_tangent"),
    m_texCoordLocationName("a_texCoord"),
    m_colorLocationName("a_color"),
    m_boneIDsLocationName("a_boneIds"),
    m_boneWeightsLocationName("a_boneWeights")
    {
        createVertexArray();
    }
    
    Mesh::~Mesh()
    {
#ifndef KINSKI_NO_VAO 
        if(m_vertexArray) GL_SUFFIX(glDeleteVertexArrays)(1, &m_vertexArray);
#endif
    }
    
    void Mesh::bindVertexPointers() const
    {
        Shader& shader = m_material->shader();
        if(!shader)
            throw Exception("No Shader defined in Mesh::createVertexArray()");
        
        // create VBOs if not yet existing
        if(!m_geometry->vertexBuffer())
            m_geometry->createGLBuffers();
        
        // define attrib pointer (vertex)
        GLuint vertexAttribLocation = shader.getAttribLocation(m_vertexLocationName);
        glBindBuffer(GL_ARRAY_BUFFER, m_geometry->vertexBuffer().id());
        glEnableVertexAttribArray(vertexAttribLocation);
        glVertexAttribPointer(vertexAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
        KINSKI_CHECK_GL_ERRORS();
        
        if(m_geometry->hasNormals())
        {
            GLint normalAttribLocation = shader.getAttribLocation(m_normalLocationName);
            
            if(normalAttribLocation >= 0)
            {
                glBindBuffer(GL_ARRAY_BUFFER, m_geometry->normalBuffer().id());
                // define attrib pointer (tangent)
                glEnableVertexAttribArray(normalAttribLocation);
                glVertexAttribPointer(normalAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
                KINSKI_CHECK_GL_ERRORS();
            }
        }
        
        if(m_geometry->hasTexCoords())
        {
            GLint texCoordAttribLocation = shader.getAttribLocation(m_texCoordLocationName);
            
            if(texCoordAttribLocation >= 0)
            {
                glBindBuffer(GL_ARRAY_BUFFER, m_geometry->texCoordBuffer().id());
                // define attrib pointer (tangent)
                glEnableVertexAttribArray(texCoordAttribLocation);
                glVertexAttribPointer(texCoordAttribLocation, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
                KINSKI_CHECK_GL_ERRORS();
            }
        }
        
        if(m_geometry->hasTangents())
        {
            GLint tangentAttribLocation = shader.getAttribLocation(m_tangentLocationName);
            
            if(tangentAttribLocation >= 0)
            {
                glBindBuffer(GL_ARRAY_BUFFER, m_geometry->tangentBuffer().id());
                // define attrib pointer (tangent)
                glEnableVertexAttribArray(tangentAttribLocation);
                glVertexAttribPointer(tangentAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
                KINSKI_CHECK_GL_ERRORS();
            }
        }
        
        if(m_geometry->hasColors())
        {
            GLint colorAttribLocation = shader.getAttribLocation(m_colorLocationName);
            
            if(colorAttribLocation >= 0)
            {
                glBindBuffer(GL_ARRAY_BUFFER, m_geometry->colorBuffer().id());
                // define attrib pointer (colors)
                glEnableVertexAttribArray(colorAttribLocation);
                glVertexAttribPointer(colorAttribLocation, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
                KINSKI_CHECK_GL_ERRORS();
            }
        }else{
            GLint colorAttribLocation = shader.getAttribLocation(m_colorLocationName);
            if(colorAttribLocation >= 0) glVertexAttrib4f(colorAttribLocation, 1.0f, 1.0f, 1.0f, 1.0f);
        }
        
        if(m_geometry->hasBones())
        {
            GLint boneIdsAttribLocation = shader.getAttribLocation(m_boneIDsLocationName);
            GLint boneWeightsAttribLocation = shader.getAttribLocation(m_boneWeightsLocationName);
            
            if(boneIdsAttribLocation >= 0 && boneWeightsAttribLocation >= 0)
            {
                glBindBuffer(GL_ARRAY_BUFFER, m_geometry->boneBuffer().id());
                // define attrib pointer (boneIDs)
                glEnableVertexAttribArray(boneIdsAttribLocation);
                
                // use ivec4 to submit bone-indices on Dekstop GL
#ifndef KINSKI_GLES
                glVertexAttribIPointer(boneIdsAttribLocation, 4, GL_INT, sizeof(gl::BoneVertexData),
                                       BUFFER_OFFSET(0));
                
                // else fall back to float vec4 for GLES2
#else
                glVertexAttribPointer(boneIdsAttribLocation, 4, GL_INT, GL_FALSE,
                                      sizeof(gl::BoneVertexData), BUFFER_OFFSET(0));
#endif
                KINSKI_CHECK_GL_ERRORS();
                
                // define attrib pointer (boneWeights)
                glEnableVertexAttribArray(boneWeightsAttribLocation);
                glVertexAttribPointer(boneWeightsAttribLocation, 4, GL_FLOAT, GL_FALSE,
                                      sizeof(gl::BoneVertexData),
                                      BUFFER_OFFSET(sizeof(glm::ivec4)));

                KINSKI_CHECK_GL_ERRORS();
            }
        }
        
        // index buffer
        if(m_geometry->hasIndices())
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_geometry->indexBuffer().id());
    }

    void Mesh::createVertexArray()
    {
        if(m_geometry->vertices().empty()) return;
        
#ifndef KINSKI_NO_VAO
        if(!m_vertexArray){ GL_SUFFIX(glGenVertexArrays)(1, &m_vertexArray); }
        GL_SUFFIX(glBindVertexArray)(m_vertexArray);
        bindVertexPointers();
        GL_SUFFIX(glBindVertexArray)(0);
        m_material_vertex_array_mapping = std::make_pair(m_material, m_vertexArray);
#endif
    }
    
    void Mesh::update(float time_delta)
    {
        m_geometry->updateAnimation(time_delta);
    }
    
    AABB Mesh::boundingBox() const
    {
        return m_geometry->boundingBox();
    }
    
    GLuint Mesh::vertexArray() const
    {
        if(m_material_vertex_array_mapping.first != m_material)
        {
            throw WrongVertexArrayDefinedException(getID());
        }
        
        return m_vertexArray;
    };
    
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
    
    void Mesh::setTexCoordLocationName(const std::string &theName)
    {
        m_texCoordLocationName = theName;
        createVertexArray();
    }
}}
