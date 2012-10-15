//
//  Geometry.h
//  kinskiGL
//
//  Created by Fabian on 8/6/12.
//
//

#ifndef __kinskiGL__Geometry__
#define __kinskiGL__Geometry__

#include <vector>
#include <glm/glm.hpp>

namespace kinski
{
namespace gl
{
    struct Face3;
    struct BoundingBox;
    
    class Geometry
    {
    public:
        inline void appendVertex(const glm::vec3 &theVert)
        { m_vertices.push_back(theVert); };
        
        void appendVertices(const std::vector<glm::vec3> &theVerts);
        void appendVertices(const glm::vec3 *theVerts, size_t numVerts);
        
        inline void appendNormal(const glm::vec3 &theVert)
        { m_normals.push_back(theVert); };
        
        void appendNormals(const std::vector<glm::vec3> &theVerts);
        void appendNormals(const glm::vec3 *theVerts, size_t numVerts);
        
        inline void appendTextCoord(float theU, float theV)
        { m_texCoords.push_back(glm::vec2(theU, theV)); };
        
        inline void appendTextCoord(const glm::vec2 &theUV)
        { m_texCoords.push_back(theUV); };
        
        void appendTextCoords(const std::vector<glm::vec2> &theVerts);
        void appendTextCoords(const glm::vec2 *theVerts, size_t numVerts);
        
        void appendFace(uint32_t a, uint32_t b, uint32_t c);
        void appendFace(const Face3 &theFace);
        
        std::vector<glm::vec3>& getVertices(){ return m_vertices; };
        const std::vector<glm::vec3>& getVertices() const { return m_vertices; };
        
        std::vector<glm::vec3>& getNormals(){ return m_normals; };
        const std::vector<glm::vec3>& getNormals() const { return m_normals; };
        
        std::vector<glm::vec2>& getTextCoords(){ return m_texCoords; };
        const std::vector<glm::vec2>& getTextCoords() const { return m_texCoords; };
        
        std::vector<Face3>& getFaces(){ return m_faces; };
        const std::vector<Face3>& getFaces() const { return m_faces; };
        
        std::vector<glm::uvec3>& getIndices(){ return m_indices; };
        const std::vector<glm::uvec3>& getIndices() const { return m_indices; };
        
    private:
        
        std::vector<glm::vec3> m_vertices;
        std::vector<glm::vec3> m_normals;
        std::vector<glm::vec2> m_texCoords;
        std::vector<Face3> m_faces;
        
        std::vector<glm::uvec3> m_indices;
        
    };
    
    // TODO: check if really needed
    struct Face3
    {
        Face3(uint32_t a, uint32_t b, uint32_t c, glm::vec3 n = glm::vec3(1)):
        m_a(a), m_b(b), m_c(c), m_normal(n)
        {};
        
        // the indices
        uint32_t m_a, m_b, m_c;
        
        glm::vec3 m_normal;
        std::vector<glm::vec3> m_vertNormals;
    };
    
    // Derived primitives
    class Plane : public Geometry
    {
    public:
        
        Plane(float width, float height,
              uint32_t numSegments_W = 1, uint32_t numSegments_H = 1);
    };
    
    class Box : public Geometry
    {
        Box();
    };
    
}//gl
}//kinski

#endif /* defined(__kinskiGL__Geometry__) */
