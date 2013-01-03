//
//  Mesh.cpp
//  kinskiGL
//
//  Created by Fabian on 11/2/12.
//
//

#include "Mesh.h"

namespace kinski { namespace gl {
  
    Mesh::Mesh(const Geometry::Ptr &theGeom, const Material::Ptr &theMaterial):
    Object3D(),
    m_geometry(theGeom),
    m_material(theMaterial),
    m_vertexArray(0),
    m_drawMode(GL_TRIANGLES),
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
        if(m_vertexArray) GL_SUFFIX(glDeleteVertexArrays)(1, &m_vertexArray);
    }
    
    void Mesh::createVertexArray()
    {
        Shader& shader = m_material->shader();
        if(!shader)
            throw Exception("No Shader defined in Mesh::createVertexArray()");
        
        if(!m_vertexArray) GL_SUFFIX(glGenVertexArrays)(1, &m_vertexArray);
        GL_SUFFIX(glBindVertexArray)(m_vertexArray);
        
        // create VBOs if not yet existing
        if(!m_geometry->vertexBuffer())
            m_geometry->createGLBuffers();
        
        // define attrib pointer (vertex)
        GLuint vertexAttribLocation = shader.getAttribLocation(m_vertexLocationName);
        glBindBuffer(GL_ARRAY_BUFFER, m_geometry->vertexBuffer().id());
        glEnableVertexAttribArray(vertexAttribLocation);
        glVertexAttribPointer(vertexAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
        
        if(m_geometry->hasNormals())
        {
            GLuint normalAttribLocation = shader.getAttribLocation(m_normalLocationName);
            
            glBindBuffer(GL_ARRAY_BUFFER, m_geometry->normalBuffer().id());
            
            // define attrib pointer (tangent)
            glEnableVertexAttribArray(normalAttribLocation);
            glVertexAttribPointer(normalAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
        }
        
        if(m_geometry->hasTexCoords())
        {
            GLuint texCoordAttribLocation = shader.getAttribLocation(m_texCoordLocationName);
            
            glBindBuffer(GL_ARRAY_BUFFER, m_geometry->texCoordBuffer().id());
            
            // define attrib pointer (tangent)
            glEnableVertexAttribArray(texCoordAttribLocation);
            glVertexAttribPointer(texCoordAttribLocation, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
        }
        
        if(m_geometry->hasTangents())
        {
            GLuint tangentAttribLocation = shader.getAttribLocation(m_tangentLocationName);
            
            glBindBuffer(GL_ARRAY_BUFFER, m_geometry->tangentBuffer().id());
            
            // define attrib pointer (tangent)
            glEnableVertexAttribArray(tangentAttribLocation);
            glVertexAttribPointer(tangentAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
        }
        
        if(m_geometry->hasColors())
        {
            GLuint colorAttribLocation = shader.getAttribLocation(m_colorLocationName);
            
            glBindBuffer(GL_ARRAY_BUFFER, m_geometry->colorBuffer().id());
            
            // define attrib pointer (colors)
            glEnableVertexAttribArray(colorAttribLocation);
            glVertexAttribPointer(colorAttribLocation, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
        }
#ifndef KINSKI_GLES 
        if(m_geometry->hasBones())
        {
            GLuint boneIdsAttribLocation = shader.getAttribLocation(m_boneIDsLocationName);
            GLuint boneWeightsAttribLocation = shader.getAttribLocation(m_boneWeightsLocationName);
            
            glBindBuffer(GL_ARRAY_BUFFER, m_geometry->boneBuffer().id());
            
            // define attrib pointer (boneIDs)
            glEnableVertexAttribArray(boneIdsAttribLocation);
            glVertexAttribIPointer(boneIdsAttribLocation, 4, GL_INT,
                                  sizeof(gl::BoneVertexData),
                                  BUFFER_OFFSET(0));
            
            // define attrib pointer (boneWeights)
            glEnableVertexAttribArray(boneWeightsAttribLocation);
            glVertexAttribPointer(boneWeightsAttribLocation, 4, GL_FLOAT, GL_FALSE,
                                  sizeof(gl::BoneVertexData),
                                  BUFFER_OFFSET(sizeof(glm::ivec4)));
            
        }
#endif
        // index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_geometry->indexBuffer().id());
        
        GL_SUFFIX(glBindVertexArray)(0);
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
    
    void Mesh::setTexCoordLocationName(const std::string &theName)
    {
        m_texCoordLocationName = theName;
        createVertexArray();
    }
}}
