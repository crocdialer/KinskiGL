// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef __gl__Geometry__
#define __gl__Geometry__

#include "gl/gl.h"
#include "geometry_types.h"
#include "Buffer.h"

namespace kinski{ namespace gl{
    
    struct KINSKI_API Face3
    {
        Face3():a(0), b(0), c(0){};
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
        ivec4 indices;
        vec4 weights;
        
        BoneVertexData():
        indices(ivec4(0)),
        weights(vec4(0)){};
    };
    
    class KINSKI_API Geometry
    {
    public:
        
        typedef std::shared_ptr<Geometry> Ptr;
        
        ~Geometry();
        
        inline void appendVertex(const vec3 &theVert)
        { vertices().push_back(theVert); };
        
        inline void appendVertices(const std::vector<vec3> &theVerts)
        {
            vertices().insert(m_vertices.end(), theVerts.begin(), theVerts.end());
        }
        inline void appendVertices(const vec3 *theVerts, size_t numVerts)
        {
            vertices().insert(m_vertices.end(), theVerts, theVerts + numVerts);
        }
        
        inline void appendNormal(const vec3 &theNormal)
        { normals().push_back(theNormal); };
        
        inline void appendNormals(const std::vector<vec3> &theNormals)
        {
            normals().insert(m_normals.end(), theNormals.begin(), theNormals.end());
        }
        inline void appendNormals(const vec3 *theNormals, size_t numNormals)
        {
            normals().insert(m_normals.end(), theNormals, theNormals + numNormals);
        }
        
        inline void appendTextCoord(float theU, float theV)
        { texCoords().push_back(vec2(theU, theV)); };
        
        inline void appendTextCoord(const vec2 &theUV)
        { texCoords().push_back(theUV); };
        
        inline void appendTextCoords(const std::vector<vec2> &theVerts)
        {
            texCoords().insert(m_texCoords.end(), theVerts.begin(), theVerts.end());
        }
        inline void appendTextCoords(const vec2 *theVerts, size_t numVerts)
        {
            texCoords().insert(m_texCoords.end(), theVerts, theVerts + numVerts);
        }
        
        inline void appendColor(const vec4 &theColor)
        { colors().push_back(theColor); };
        
        inline void appendColors(const std::vector<vec4> &theColors)
        {
            colors().insert(m_colors.end(), theColors.begin(), theColors.end());
        }
        
        inline void appendColors(const vec4 *theColors, size_t numColors)
        {
            colors().insert(m_colors.end(), theColors, theColors + numColors);
        }
        
        inline void appendIndex(uint32_t theIndex)
        { indices().push_back(theIndex); };
        
        inline void appendIndices(const std::vector<uint32_t> &theIndices)
        {
            indices().insert(m_indices.end(), theIndices.begin(), theIndices.end());
        }
        inline void appendIndices(const uint32_t *theIndices, size_t numIndices)
        {
            indices().insert(m_indices.end(), theIndices, theIndices + numIndices);
        }
        
        inline void appendFace(uint32_t a, uint32_t b, uint32_t c){ appendFace(Face3(a, b, c)); }
        inline void appendFace(const Face3 &theFace)
        {
            m_faces.push_back(theFace);
            appendIndices(theFace.indices, 3);
        }
        inline void appendFaces(const std::vector<gl::Face3> &the_faces)
        {
            m_faces.insert(m_faces.end(), the_faces.begin(), the_faces.end());
            uint32_t *start = (uint32_t*) &the_faces[0].indices;
            m_indices.insert(m_indices.end(), start, start + 3 * the_faces.size());
        }
        
        void computeBoundingBox();
        void computeFaceNormals();
        void computeVertexNormals();
        void computeTangents();
        
        GLenum indexType();
        inline GLenum primitiveType() const {return m_primitiveType;};
        void setPrimitiveType(GLenum type){ m_primitiveType = type; };
        
        inline std::vector<vec3>& vertices(){ m_dirty_vertexBuffer = true; return m_vertices;};
        inline const std::vector<vec3>& vertices() const { return m_vertices; };
        
        bool hasNormals() const { return m_vertices.size() == m_normals.size(); };
        inline std::vector<vec3>& normals(){ m_dirty_normalBuffer = true; return m_normals; };
        inline const std::vector<vec3>& normals() const { return m_normals; };
        
        bool hasTangents() const { return m_vertices.size() == m_tangents.size(); };
        inline std::vector<vec3>& tangents(){ m_dirty_tangentBuffer = true; return m_tangents; };
        inline const std::vector<vec3>& tangents() const { return m_tangents; };
        
        bool hasPointSizes() const { return m_vertices.size() == m_point_sizes.size(); };
        inline std::vector<float>& point_sizes(){ m_dirty_pointSizeBuffer = true; return m_point_sizes; };
        inline const std::vector<float>& point_sizes() const { return m_point_sizes; };
        
        bool hasTexCoords() const { return m_vertices.size() == m_texCoords.size(); };
        inline std::vector<vec2>& texCoords(){ m_dirty_texCoordBuffer = true; return m_texCoords; };
        inline const std::vector<vec2>& texCoords() const { return m_texCoords; };
        
        bool hasColors() const { return m_vertices.size() == m_colors.size(); };
        std::vector<vec4>& colors(){ m_dirty_colorBuffer = true; return m_colors; };
        const std::vector<vec4>& colors() const { return m_colors; };
        
        bool hasIndices() const { return !m_indices.empty(); };
        std::vector<uint32_t>& indices(){ m_dirty_indexBuffer = true; return m_indices; };
        const std::vector<uint32_t>& indices() const { return m_indices; };
        
        inline std::vector<Face3>& faces(){ return m_faces; };
        inline const std::vector<Face3>& faces() const { return m_faces; };
        
        bool hasBones() const { return m_vertices.size() == m_boneVertexData.size(); };
        std::vector<BoneVertexData>& boneVertexData(){ return m_boneVertexData; };
        const std::vector<BoneVertexData>& boneVertexData() const { return m_boneVertexData; };
        
        inline const AABB& boundingBox() const { return m_boundingBox; };
        
        // GL buffers
        gl::Buffer& vertexBuffer(){ return m_vertexBuffer; };
        gl::Buffer& normalBuffer(){ return m_normalBuffer; };
        gl::Buffer& texCoordBuffer(){ return m_texCoordBuffer; };
        gl::Buffer& tangentBuffer(){ return m_tangentBuffer; };
        gl::Buffer& pointSizeBuffer(){ return m_pointSizeBuffer; };
        gl::Buffer& colorBuffer(){ return m_colorBuffer; };
        gl::Buffer& boneBuffer(){ return m_boneBuffer; };
        gl::Buffer& indexBuffer(){ return m_indexBuffer; };
        const gl::Buffer& vertexBuffer() const { return m_vertexBuffer; };
        const gl::Buffer& normalBuffer() const { return m_normalBuffer; };
        const gl::Buffer& texCoordBuffer() const { return m_texCoordBuffer; };
        const gl::Buffer& tangentBuffer() const { return m_tangentBuffer; };
        const gl::Buffer& pointSizeBuffer() const { return m_pointSizeBuffer; };
        const gl::Buffer& colorBuffer() const { return m_colorBuffer; };
        const gl::Buffer& boneBuffer() const { return m_boneBuffer; };
        const gl::Buffer& indexBuffer() const { return m_indexBuffer; };
        
        bool has_dirty_buffers() const;
        void createGLBuffers();
        
        /********************************* Factory methods ****************************************/
         
        static GeometryPtr create(){return Ptr(new Geometry());};
        static GeometryPtr createPlane(float width, float height,
                                       uint32_t numSegments_W = 1,
                                       uint32_t numSegments_H = 1);
        static GeometryPtr createSolidUnitCircle(int numSegments);
        static GeometryPtr createUnitCircle(int numSegments);
        static GeometryPtr createBox(const vec3 &theHalfExtents);
        static GeometryPtr createSphere(float radius, int numSlices);
        static GeometryPtr createCone(float radius, float height, int numSegments);
        
    private:
        
        Geometry();
        
        // defaults to GL_TRIANGLES
        GLenum m_primitiveType;
        
        std::vector<vec3> m_vertices;
        std::vector<vec3> m_normals;
        std::vector<vec2> m_texCoords;
        std::vector<vec4> m_colors;
        std::vector<vec3> m_tangents;
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
        
        bool m_dirty_vertexBuffer;
        bool m_dirty_normalBuffer;
        bool m_dirty_texCoordBuffer;
        bool m_dirty_colorBuffer;
        bool m_dirty_tangentBuffer;
        bool m_dirty_pointSizeBuffer;
        bool m_dirty_indexBuffer;
        bool m_dirty_boneBuffer;
    };

}//gl
}//kinski

#endif /* defined(__gl__Geometry__) */
