// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef __kinskiGL__Geometry__
#define __kinskiGL__Geometry__

#include "KinskiGL.h"
#include "geometry_types.h"
#include "Buffer.h"

namespace kinski{ namespace gl{
    
    struct KINSKI_API Face3
    {
        Face3(){};
        Face3(uint32_t theA, uint32_t theB, uint32_t theC):
        a(theA), b(theB), c(theC){}
        
        // vertex indices
        union
        {
            struct{uint32_t a, b, c;};
            uint32_t indices[3];
        };
    };
    
    // each vertex can reference up to 4 bones
    struct KINSKI_API BoneVertexData
    {
        glm::ivec4 indices;
        glm::vec4 weights;
        
        BoneVertexData():
        indices(glm::ivec4(0)),
        weights(glm::vec4(0)){};
    };
    
    class KINSKI_API Geometry
    {
    public:
        
        typedef std::shared_ptr<Geometry> Ptr;
        
        static Ptr create(){return Ptr(new Geometry());};
        ~Geometry();
        
        inline void appendVertex(const glm::vec3 &theVert)
        { m_vertices.push_back(theVert); };
        
        inline void appendVertices(const std::vector<glm::vec3> &theVerts)
        {
            m_vertices.insert(m_vertices.end(), theVerts.begin(), theVerts.end());
        }
        inline void appendVertices(const glm::vec3 *theVerts, size_t numVerts)
        {
            m_vertices.insert(m_vertices.end(), theVerts, theVerts + numVerts);
        }
        
        inline void appendNormal(const glm::vec3 &theNormal)
        { m_normals.push_back(theNormal); };
        
        inline void appendNormals(const std::vector<glm::vec3> &theNormals)
        {
            m_normals.insert(m_normals.end(), theNormals.begin(), theNormals.end());
        }
        inline void appendNormals(const glm::vec3 *theNormals, size_t numNormals)
        {
            m_normals.insert(m_normals.end(), theNormals, theNormals + numNormals);
        }
        
        inline void appendTextCoord(float theU, float theV)
        { m_texCoords.push_back(glm::vec2(theU, theV)); };
        
        inline void appendTextCoord(const glm::vec2 &theUV)
        { m_texCoords.push_back(theUV); };
        
        inline void appendTextCoords(const std::vector<glm::vec2> &theVerts)
        {
            m_texCoords.insert(m_texCoords.end(), theVerts.begin(), theVerts.end());
        }
        inline void appendTextCoords(const glm::vec2 *theVerts, size_t numVerts)
        {
            m_texCoords.insert(m_texCoords.end(), theVerts, theVerts + numVerts);
        }
        
        inline void appendColor(const glm::vec4 &theColor)
        { m_colors.push_back(theColor); };
        
        inline void appendColors(const std::vector<glm::vec4> &theColors)
        {
            m_colors.insert(m_colors.end(), theColors.begin(), theColors.end());
        }
        
        inline void appendColors(const glm::vec4 *theColors, size_t numColors)
        {
            m_colors.insert(m_colors.end(), theColors, theColors + numColors);
        }
        
        inline void appendIndex(uint32_t theIndex)
        { m_indices.push_back(theIndex); };
        
        inline void appendIndices(const std::vector<uint32_t> &theIndices)
        {
            m_indices.insert(m_indices.end(), theIndices.begin(), theIndices.end());
        }
        inline void appendIndices(const uint32_t *theIndices, size_t numIndices)
        {
            m_indices.insert(m_indices.end(), theIndices, theIndices + numIndices);
        }
        
        inline void appendFace(uint32_t a, uint32_t b, uint32_t c){ appendFace(Face3(a, b, c)); }
        inline void appendFace(const Face3 &theFace)
        {
            m_faces.push_back(theFace);
            appendIndices(theFace.indices, 3);
        }
        
        void computeBoundingBox();
        void computeFaceNormals();
        void computeVertexNormals();
        void computeTangents();
        
        GLenum indexType();
        inline GLenum primitiveType() const {return m_primitiveType;};
        void setPrimitiveType(GLenum type){ m_primitiveType = type; };
        
        inline std::vector<glm::vec3>& vertices(){ return m_vertices; };
        inline const std::vector<glm::vec3>& vertices() const { return m_vertices; };
        
        bool hasNormals() const { return m_vertices.size() == m_normals.size(); };
        inline std::vector<glm::vec3>& normals(){ return m_normals; };
        inline const std::vector<glm::vec3>& normals() const { return m_normals; };
        
        bool hasTangents() const { return m_vertices.size() == m_tangents.size(); };
        inline std::vector<glm::vec3>& tangents(){ return m_tangents; };
        inline const std::vector<glm::vec3>& tangents() const { return m_tangents; };
        
        bool hasPointSizes() const { return m_vertices.size() == m_point_sizes.size(); };
        inline std::vector<float>& point_sizes(){ return m_point_sizes; };
        inline const std::vector<float>& point_sizes() const { return m_point_sizes; };
        
        bool hasTexCoords() const { return m_vertices.size() == m_texCoords.size(); };
        inline std::vector<glm::vec2>& texCoords(){ return m_texCoords; };
        inline const std::vector<glm::vec2>& texCoords() const { return m_texCoords; };
        
        bool hasColors() const { return m_vertices.size() == m_colors.size(); };
        std::vector<glm::vec4>& colors(){ return m_colors; };
        const std::vector<glm::vec4>& colors() const { return m_colors; };
        
        bool hasIndices() const { return !m_indices.empty(); };
        std::vector<uint32_t>& indices(){ return m_indices; };
        const std::vector<uint32_t>& indices() const { return m_indices; };
        
        inline std::vector<Face3>& faces(){ return m_faces; };
        inline const std::vector<Face3>& faces() const { return m_faces; };
        
        bool hasBones() const { return m_vertices.size() == m_boneVertexData.size(); };
        std::vector<BoneVertexData>& boneVertexData(){ return m_boneVertexData; };
        const std::vector<BoneVertexData>& boneVertexData() const { return m_boneVertexData; };
        
        inline const AABB& boundingBox() const { return m_boundingBox; };
        
        // GL buffers
        const gl::Buffer& vertexBuffer() const { return m_vertexBuffer; };
        const gl::Buffer& normalBuffer() const { return m_normalBuffer; };
        const gl::Buffer& texCoordBuffer() const { return m_texCoordBuffer; };
        const gl::Buffer& tangentBuffer() const { return m_tangentBuffer; };
        const gl::Buffer& pointSizeBuffer() const { return m_pointSizeBuffer; };
        const gl::Buffer& colorBuffer() const { return m_colorBuffer; };
        const gl::Buffer& boneBuffer() const { return m_boneBuffer; };
        const gl::Buffer& indexBuffer() const { return m_indexBuffer; };
        
        void createGLBuffers();
        
    private:
        
        Geometry();
        
        // defaults to GL_TRIANGLES
        GLenum m_primitiveType;
        
        std::vector<glm::vec3> m_vertices;
        std::vector<glm::vec3> m_normals;
        std::vector<glm::vec2> m_texCoords;
        std::vector<glm::vec4> m_colors;
        std::vector<glm::vec3> m_tangents;
        std::vector<float> m_point_sizes;
        std::vector<uint32_t> m_indices;
        std::vector<Face3> m_faces;
        std::vector<BoneVertexData> m_boneVertexData;
        
        AABB m_boundingBox;
        gl::Buffer m_vertexBuffer;
        gl::Buffer m_normalBuffer;
        gl::Buffer m_texCoordBuffer;
        gl::Buffer m_colorBuffer;
        gl::Buffer m_tangentBuffer;
        gl::Buffer m_pointSizeBuffer;
        gl::Buffer m_indexBuffer;
        gl::Buffer m_boneBuffer;
        
        bool m_needsUpdate;
    };
    
    GeometryPtr createPlane(float width, float height,
                            uint32_t numSegments_W = 1,
                            uint32_t numSegments_H = 1);
    
    GeometryPtr createBox(const glm::vec3 &theHalfExtents);
    
    GeometryPtr createSphere(float radius, int numSlices);
    
}//gl
}//kinski

#endif /* defined(__kinskiGL__Geometry__) */
