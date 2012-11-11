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
    m_texCoordLocationName("a_texCoord")
    {
        createVertexArray();
    }
    
    Mesh::~Mesh()
    {
        if(m_vertexArray) glDeleteVertexArrays(1, &m_vertexArray);
    }
    
    void Mesh::createVertexArray()
    {
        Shader& shader = m_material->getShader();
        if(!shader)
            throw Exception("No Shader defined in Mesh::createVertexArray()");
        
        if(!m_vertexArray) glGenVertexArrays(1, &m_vertexArray);
        glBindVertexArray(m_vertexArray);
        
        GLuint vertexAttribLocation = shader.getAttribLocation(m_vertexLocationName);
        GLuint normalAttribLocation = shader.getAttribLocation(m_normalLocationName);
        GLuint texCoordAttribLocation = shader.getAttribLocation(m_texCoordLocationName);
        
        uint32_t numFloats = m_geometry->getNumComponents();
        GLsizei stride = numFloats * sizeof(GLfloat);
        
        glBindBuffer(GL_ARRAY_BUFFER, m_geometry->getInterleavedBuffer());
        
        // define attrib pointer (texCoord)
        glEnableVertexAttribArray(texCoordAttribLocation);
        glVertexAttribPointer(texCoordAttribLocation, 2, GL_FLOAT, GL_FALSE,
                              stride,
                              BUFFER_OFFSET(0));
        
        // define attrib pointer (normal)
        glEnableVertexAttribArray(normalAttribLocation);
        glVertexAttribPointer(normalAttribLocation, 3, GL_FLOAT, GL_FALSE,
                              stride,
                              BUFFER_OFFSET(2 * sizeof(GLfloat)));
        
        // define attrib pointer (vertex)
        glEnableVertexAttribArray(vertexAttribLocation);
        glVertexAttribPointer(vertexAttribLocation, 3, GL_FLOAT, GL_FALSE,
                              stride,
                              BUFFER_OFFSET(5 * sizeof(GLfloat)));
        
        // index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_geometry->getIndexBuffer());
        
        glBindVertexArray(0);
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

    void Mesh::setTexCoordLocationName(const std::string &theName)
    {
        m_texCoordLocationName = theName;
        createVertexArray();
    }
}}