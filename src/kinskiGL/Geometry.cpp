//
//  Geometry.cpp
//  kinskiGL
//
//  Created by Fabian on 8/6/12.
//
//

#include "Geometry.h"

using namespace std;

namespace kinski{ namespace gl{
    
    void Geometry::appendVertices(const std::vector<glm::vec3> &theVerts)
    {
        m_vertices.reserve(m_vertices.size() + theVerts.size());
        
        m_vertices.insert(m_vertices.end(), theVerts.begin(), theVerts.end());
    }
    
    void Geometry::appendVertices(const glm::vec3 *theVerts, size_t numVerts)
    {
        m_vertices.reserve(m_vertices.size() + numVerts);
        
        m_vertices.insert(m_vertices.end(), theVerts, theVerts + numVerts);
    }
    
    void Geometry::appendTextCoords(const std::vector<glm::vec2> &theVerts)
    {
        m_texCoords.reserve(m_texCoords.size() + theVerts.size());
        
        m_texCoords.insert(m_texCoords.end(), theVerts.begin(), theVerts.end());
    }
    
    void Geometry::appendTextCoords(const glm::vec2 *theVerts, size_t numVerts)
    {
        m_texCoords.reserve(m_texCoords.size() + numVerts);
        
        m_texCoords.insert(m_texCoords.end(), theVerts, theVerts + numVerts);

    }
    
    void Geometry::appendFace(uint32_t a, uint32_t b, uint32_t c)
    {
        appendFace(Face3(a, b, c));
    }
    
    void Geometry::appendFace(const Face3 &theFace)
    {
        m_faces.push_back(theFace);
    }
    
    void Geometry::computeBoundingBox()
    {
        m_boundingBox = BoundingBox();
     
        vector<glm::vec3>::const_iterator it = m_vertices.begin();
        for (; it != m_vertices.end(); it++)
        {
            const glm::vec3 &vertex = *it;
            
            // X
            if(vertex.x < m_boundingBox.min.x)
                m_boundingBox.min.x = vertex.x;
            else if(vertex.x > m_boundingBox.max.x)
                m_boundingBox.max.x = vertex.x;
            
            // Y
            if(vertex.y < m_boundingBox.min.y)
                m_boundingBox.min.y = vertex.y;
            else if(vertex.y > m_boundingBox.max.y)
                m_boundingBox.max.y = vertex.y;
            
            // Z
            if(vertex.z < m_boundingBox.min.z)
                m_boundingBox.min.z = vertex.z;
            else if(vertex.z > m_boundingBox.max.z)
                m_boundingBox.max.z = vertex.z;
        }
    }
    
    void Geometry::computeFaceNormals()
    {
        std::vector<Face3>::iterator it = m_faces.begin();
        
        for (; it != m_faces.end(); it++)
        {
            Face3 &face = *it;
            
            const glm::vec3 &vA = m_vertices[ face.a ];
			const glm::vec3 &vB = m_vertices[ face.b ];
			const glm::vec3 &vC = m_vertices[ face.c ];
            
			face.normal = glm::normalize(glm::cross(vB - vA, vC - vA));
            
            face.vertexNormals[0] = face.normal;
            face.vertexNormals[1] = face.normal;
            face.vertexNormals[2] = face.normal;
        }
    }
    
    void Geometry::computeVertexNormals()
    {
        // compute face normals first
        computeFaceNormals();
        
        // create tmp array, if not yet constructed
        if(m_tmpVertexNormals.size() != m_vertices.size())
        {
            m_tmpVertexNormals.clear();
            m_tmpVertexNormals.reserve(m_vertices.size());
            
            for (int i = 0; i < m_vertices.size(); i++)
            {
                m_tmpVertexNormals.push_back(glm::vec3(0));
            }
        }
        else
        {
            std::fill(m_tmpVertexNormals.begin(), m_tmpVertexNormals.end(), glm::vec3(0));
        }
        
        // iterate faces and sum normals for all vertices
        vector<Face3>::iterator faceIt = m_faces.begin();
        for (; faceIt != m_faces.end(); faceIt++)
        {
            const Face3 &face = *faceIt;
            
            m_tmpVertexNormals[face.a] += face.normal;
            m_tmpVertexNormals[face.b] += face.normal;
            m_tmpVertexNormals[face.c] += face.normal;
        }
        
        // normalize vertexNormals
        vector<glm::vec3>::iterator normIt = m_tmpVertexNormals.begin();
        for (; normIt != m_tmpVertexNormals.end(); normIt++)
        {
            glm::vec3 &vertNormal = *normIt;
            vertNormal = glm::normalize(vertNormal);
        }
        
        // iterate faces again to fill in normals
        for (faceIt = m_faces.begin(); faceIt != m_faces.end(); faceIt++)
        {
            Face3 &face = *faceIt;
            face.vertexNormals[0] = m_tmpVertexNormals[face.a];
            face.vertexNormals[1] = m_tmpVertexNormals[face.b];
            face.vertexNormals[2] = m_tmpVertexNormals[face.c];
            
        }
    }
    
    /********************************* PRIMITIVES ****************************************/
    
    Plane::Plane(float width, float height,
                 uint32_t numSegments_W, uint32_t numSegments_H)
    :Geometry()
    {
        float width_half = width / 2, height_half = height / 2;
        float segment_width = width / numSegments_W, segment_height = height / numSegments_H;
        
        uint32_t gridX = numSegments_W, gridZ = numSegments_H, gridX1 = gridX +1, gridZ1 = gridZ + 1;
        
        glm::vec3 normal (0, 0, 1);
        
        // create vertices
        for ( uint32_t iz = 0; iz < gridZ1; iz ++ )
        {
        
            for ( uint32_t ix = 0; ix < gridX1; ix ++ )
            {
        
                float x = ix * segment_width - width_half;
                float y = iz * segment_height - height_half;
        
                appendVertex( glm::vec3( x, - y, 0) );
                appendTextCoord(ix / (float)gridX, - iz / (float)gridZ);
                
            }
        }
        
        std::vector<glm::vec3> vertNormals;
        vertNormals.push_back(normal);
        vertNormals.push_back(normal);
        vertNormals.push_back(normal);
        
        // create faces and texcoords
        for ( uint32_t iz = 0; iz < gridZ1; iz ++ )
        {
            for ( uint32_t ix = 0; ix < gridX1; ix ++ )
            {
                uint32_t a = ix + gridX1 * iz;
                uint32_t b = ix + gridX1 * ( iz + 1);
                uint32_t c = ( ix + 1 ) + gridX1 * ( iz + 1 );
                uint32_t d = ( ix + 1 ) + gridX1 * iz;
                
                Face3 f1(a, b, d), f2(b, c, d);
                f1.normal = normal;
                f2.normal = normal;
                
                f1.vertexNormals = vertNormals;
                f2.vertexNormals = vertNormals;
                
                appendFace(f1);
                appendFace(f2);
            }
        }

    }
    
}}//namespace
