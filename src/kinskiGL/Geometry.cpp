//
//  Geometry.cpp
//  kinskiGL
//
//  Created by Fabian on 8/6/12.
//
//

#include "Geometry.h"

namespace kinski
{
namespace gl
{
    void Geometry::appendVertices(const std::vector<glm::vec3> &theVerts)
    {
        m_vertices.insert(m_vertices.end(), theVerts.begin(), theVerts.end());
    }
    
    void Geometry::appendVertices(const glm::vec3 *theVerts, size_t numVerts)
    {
        m_vertices.reserve(m_vertices.size() + numVerts);
        
        const glm::vec3 *v = theVerts, *end = theVerts + numVerts;
        m_vertices.insert(m_vertices.end(), v, end);
    }
    
    void Geometry::appendNormals(const std::vector<glm::vec3> &theVerts)
    {
        m_normals.insert(m_normals.end(), theVerts.begin(), theVerts.end());
    }
    
    void Geometry::appendNormals(const glm::vec3 *theVerts, size_t numVerts)
    {
        m_normals.reserve(m_normals.size() + numVerts);
        
        const glm::vec3 *v = theVerts, *end = theVerts + numVerts;
        m_normals.insert(m_normals.end(), v, end);
    }
    
    void Geometry::appendTextCoords(const std::vector<glm::vec2> &theVerts)
    {
        m_texCoords.insert(m_texCoords.end(), theVerts.begin(), theVerts.end());
    }
    
    void Geometry::appendTextCoords(const glm::vec2 *theVerts, size_t numVerts)
    {
        m_texCoords.reserve(m_texCoords.size() + numVerts);
        
        const glm::vec2 *v = theVerts, *end = theVerts + numVerts;
        m_texCoords.insert(m_texCoords.end(), v, end);

    }
    
    void Geometry::appendFace(uint32_t a, uint32_t b, uint32_t c)
    {
        appendFace(Face3(a, b, c));
    }
    
    void Geometry::appendFace(const Face3 &theFace)
    {
        m_faces.push_back(theFace);
        m_indices.push_back(glm::uvec3(theFace.m_a, theFace.m_b, theFace.m_c));
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

        appendNormal(normal);
        
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
                
                Face3 f1(a, b, c), f2(c, d, a);
                f1.m_normal = normal;
                f2.m_normal = normal;
                
                f1.m_vertNormals = vertNormals;
                f2.m_vertNormals = vertNormals;
                
                appendFace(f1);
                appendFace(f2);
                
//                appendTextCoord(ix / (float)gridX, 1 - iz / (float)gridZ);
//                appendTextCoord(ix / (float)gridX, 1 - (iz + 1) / (float)gridZ);
//                appendTextCoord( (ix + 1) / (float)gridX, 1 - (iz + 1) / (float)gridZ);
//                appendTextCoord( (ix + 1) / (float)gridX, 1 - iz / (float)gridZ);
            }
        }

    }
    
}//gl
}//kinski
