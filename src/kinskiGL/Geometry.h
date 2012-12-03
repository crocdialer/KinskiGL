//
//  Geometry.h
//  kinskiGL
//
//  Created by Fabian on 8/6/12.
//
//

#ifndef __kinskiGL__Geometry__
#define __kinskiGL__Geometry__

#include "KinskiGL.h"

namespace kinski
{
namespace gl
{
    struct Face3
    {
        Face3(uint32_t theA, uint32_t theB, uint32_t theC, glm::vec3 theNormal = glm::vec3(0)):
        a(theA), b(theB), c(theC), normal(theNormal)
        {
            vertexNormals.push_back(glm::vec3(0));
            vertexNormals.push_back(glm::vec3(0));
            vertexNormals.push_back(glm::vec3(0));
        };
        
        // vertex indices
        union
        {
            struct{uint32_t a, b, c;};
            uint32_t indices[3];
        };
        
        // texCoord indices
        union
        {
            struct{uint32_t uv_a, uv_b, uv_c;};
            uint32_t uv_indices[3];
        };
        
        glm::vec3 normal;
        std::vector<glm::vec3> vertexNormals;
    };
    
    struct BoundingBox
    {
        glm::vec3 min, max;
        
        BoundingBox(const glm::vec3 &theMin, const glm::vec3 &theMax):
        min(theMin),max(theMax)
        {};
    };
    
    // each vertex can reference up to 4 bones
    struct BoneVertexData
    {
        glm::ivec4 indices;
        glm::vec4 weights;
        
        BoneVertexData():
        indices(glm::ivec4(0)),
        weights(glm::vec4(0)){};
    };
    
    struct AnimationFrame
    {
        float time;
        std::vector<glm::mat4> boneTransforms;
    };
    
    struct Animation
    {
        float duration;
        float ticksPerSec;
        std::vector<AnimationFrame> frames;
    };
    
    class Geometry
    {
    public:
        
        typedef std::shared_ptr<Geometry> Ptr;
        
        Geometry();
        virtual ~Geometry();
        
        inline void appendVertex(const glm::vec3 &theVert)
        { m_vertices.push_back(theVert); };
        
        void appendVertices(const std::vector<glm::vec3> &theVerts);
        void appendVertices(const glm::vec3 *theVerts, size_t numVerts);
        
        inline void appendNormal(const glm::vec3 &theNormal)
        { m_normals.push_back(theNormal); };
        
        void appendNormals(const std::vector<glm::vec3> &theNormals);
        void appendNormals(const glm::vec3 *theVerts, size_t numNormals);
        
        inline void appendTextCoord(float theU, float theV)
        { m_texCoords.push_back(glm::vec2(theU, theV)); };
        
        inline void appendTextCoord(const glm::vec2 &theUV)
        { m_texCoords.push_back(theUV); };
        
        void appendTextCoords(const std::vector<glm::vec2> &theVerts);
        void appendTextCoords(const glm::vec2 *theVerts, size_t numVerts);
        
        void appendFace(uint32_t a, uint32_t b, uint32_t c);
        void appendFace(const Face3 &theFace);
        
        void computeBoundingBox();
        
        void computeFaceNormals();
        
        void computeVertexNormals();
        
        inline std::vector<glm::vec3>& getVertices(){ return m_vertices; };
        inline const std::vector<glm::vec3>& getVertices() const { return m_vertices; };
        
        inline std::vector<glm::vec3>& getNormals(){ return m_normals; };
        inline const std::vector<glm::vec3>& getNormals() const { return m_normals; };
        
        inline std::vector<glm::vec3>& getTangents(){ return m_tangents; };
        inline const std::vector<glm::vec3>& getTangents() const { return m_tangents; };
        
        inline std::vector<glm::vec2>& getTexCoords(){ return m_texCoords; };
        inline const std::vector<glm::vec2>& getTexCoords() const { return m_texCoords; };
        
        inline std::vector<Face3>& getFaces(){ return m_faces; };
        inline const std::vector<Face3>& getFaces() const { return m_faces; };
        
        std::vector<glm::mat4>& getBoneMatrices(){ return m_boneMatrices; };
        const std::vector<glm::mat4>& getBoneMatrices() const { return m_boneMatrices; };
        
        std::vector<BoneVertexData>& getBoneData(){ return m_boneData; };
        const std::vector<BoneVertexData>& getBoneData() const { return m_boneData; };
        
        bool hasBones() const { return m_vertices.size() == m_boneData.size(); };
        void setAnimation(const std::shared_ptr<Animation> &theAnim) { m_animation = theAnim; };
        
        inline const BoundingBox& getBoundingBox() const { return m_boundingBox; };
        
        GLuint getInterleavedBuffer() const { return m_interleavedBuffer; };
        GLuint getBoneBuffer() const { return m_boneBuffer; };
        GLuint getIndexBuffer() const { return m_indexBuffer; };
        
        //number of float components (per vertex) in interleaved buffer
        GLuint getNumComponents();
        
        void createGLBuffers();
        
    private:
        
        std::vector<glm::vec3> m_vertices;
        std::vector<glm::vec3> m_normals;
        std::vector<glm::vec3> m_tangents;
        
        std::vector<glm::vec2> m_texCoords;
        std::vector<Face3> m_faces;
        
        // skeletal animations stuff
        std::vector<glm::mat4> m_boneMatrices;
        std::vector<BoneVertexData> m_boneData;
        std::shared_ptr<Animation> m_animation;
        
        
        BoundingBox m_boundingBox;
        
        GLuint m_interleavedBuffer;
        GLuint m_boneBuffer;
        GLuint m_indexBuffer;
        
        bool m_needsUpdate;
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
    public:
        
        Box();
    };
    
}//gl
}//kinski

#endif /* defined(__kinskiGL__Geometry__) */
