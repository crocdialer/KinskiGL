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
#include "Buffer.h"

namespace kinski{ namespace gl{
    
    struct Face3
    {
        Face3(uint32_t theA, uint32_t theB, uint32_t theC, glm::vec3 theNormal = glm::vec3(0)):
        a(theA), b(theB), c(theC), normal(theNormal)
        {
            vertexNormals.push_back(theNormal);
            vertexNormals.push_back(theNormal);
            vertexNormals.push_back(theNormal);
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
        glm::vec3 tangent;
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
    
    struct Bone
    {
        std::string name;
        
        glm::mat4 transform;
        glm::mat4 worldtransform;
        glm::mat4 offset;
        
        uint32_t index;
        
        std::shared_ptr<Bone> parent;
        std::list<std::shared_ptr<Bone> > children;
    };
    
    template<typename T> struct Key
    {
        float time;
        T value;
        
        Key(float t, const T &v):time(t), value(v){};
    };
    
    struct AnimationKeys
    {
        std::vector< Key<glm::vec3> > positionkeys;
        std::vector< Key<glm::quat> > rotationkeys;
        std::vector< Key<glm::vec3> > scalekeys;
    };

    struct Animation
    {
        float duration;
        float ticksPerSec;
        std::map<std::shared_ptr<Bone>, AnimationKeys> boneKeys;
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
        
        inline void appendColor(const glm::vec4 &theColor)
        { m_colors.push_back(theColor); };
        
        void appendColors(const std::vector<glm::vec4> &theColors);
        void appendColors(const glm::vec4 *theColors, size_t numColors);
        
        void appendFace(uint32_t a, uint32_t b, uint32_t c);
        void appendFace(const Face3 &theFace);
        
        void computeBoundingBox();
        
        void computeFaceNormals();
        
        void computeVertexNormals();
        
        void computeTangents();
        
        inline GLenum primitiveType() const {return m_primitiveType;};
        
        inline std::vector<glm::vec3>& vertices(){ return m_vertices; };
        inline const std::vector<glm::vec3>& vertices() const { return m_vertices; };
        
        bool hasNormals() const { return m_vertices.size() == m_normals.size(); };
        inline std::vector<glm::vec3>& normals(){ return m_normals; };
        inline const std::vector<glm::vec3>& normals() const { return m_normals; };
        
        bool hasTangents() const { return m_vertices.size() == m_tangents.size(); };
        inline std::vector<glm::vec3>& tangents(){ return m_tangents; };
        inline const std::vector<glm::vec3>& tangents() const { return m_tangents; };
        
        bool hasTexCoords() const { return m_vertices.size() == m_texCoords.size(); };
        inline std::vector<glm::vec2>& texCoords(){ return m_texCoords; };
        inline const std::vector<glm::vec2>& texCoords() const { return m_texCoords; };
        
        bool hasColors() const { return m_vertices.size() == m_colors.size(); };
        std::vector<glm::vec4>& colors(){ return m_colors; };
        const std::vector<glm::vec4>& colors() const { return m_colors; };
        
        inline std::vector<Face3>& faces(){ return m_faces; };
        inline const std::vector<Face3>& faces() const { return m_faces; };
        
        std::vector<glm::mat4>& boneMatrices(){ return m_boneMatrices; };
        const std::vector<glm::mat4>& boneMatrices() const { return m_boneMatrices; };
        
        std::shared_ptr<Bone>& rootBone(){ return m_rootBone; };
        const std::shared_ptr<Bone>& rootBone() const { return m_rootBone; };
        
        bool hasBones() const { return m_vertices.size() == m_boneVertexData.size(); };
        std::vector<BoneVertexData>& boneVertexData(){ return m_boneVertexData; };
        const std::vector<BoneVertexData>& boneVertexData() const { return m_boneVertexData; };

        const std::shared_ptr<const Animation> animation() const { return m_animation; };
        std::shared_ptr<Animation> animation() { return m_animation; };
        
        void setAnimation(const std::shared_ptr<Animation> &theAnim) { m_animation = theAnim; };
        
        inline const BoundingBox& boundingBox() const { return m_boundingBox; };
        
        // GL buffers
        const gl::Buffer& vertexBuffer() const { return m_vertexBuffer; };
        const gl::Buffer& normalBuffer() const { return m_normalBuffer; };
        const gl::Buffer& texCoordBuffer() const { return m_texCoordBuffer; };
        const gl::Buffer& tangentBuffer() const { return m_tangentBuffer; };
        const gl::Buffer& colorBuffer() const { return m_colorBuffer; };
        const gl::Buffer& boneBuffer() const { return m_boneBuffer; };
        const gl::Buffer& indexBuffer() const { return m_indexBuffer; };
        
        void createGLBuffers();
        
        void updateAnimation(float time);
        
    private:
        
        void buildBoneMatrices(float time, std::shared_ptr<Bone> bone,
                               glm::mat4 parentTransform,
                               std::vector<glm::mat4> &matrices);
        
        // defaults to GL_TRIANGLES
        GLenum m_primitiveType;
        
        std::vector<glm::vec3> m_vertices;
        std::vector<glm::vec3> m_normals;
        std::vector<glm::vec2> m_texCoords;
        std::vector<glm::vec4> m_colors;
        std::vector<glm::vec3> m_tangents;
        std::vector<Face3> m_faces;
        
        // skeletal animations stuff
        std::vector<glm::mat4> m_boneMatrices;
        std::vector<BoneVertexData> m_boneVertexData;
        std::shared_ptr<Animation> m_animation;
        std::shared_ptr<Bone> m_rootBone;
        
        
        BoundingBox m_boundingBox;
        
        gl::Buffer m_vertexBuffer;
        gl::Buffer m_normalBuffer;
        gl::Buffer m_texCoordBuffer;
        gl::Buffer m_tangentBuffer;
        gl::Buffer m_boneBuffer;
        gl::Buffer m_colorBuffer;
        gl::Buffer m_indexBuffer;
        
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
        
        Box(glm::vec3 theHalfExtents = glm::vec3(0.5f));
    };
    
}//gl
}//kinski

#endif /* defined(__kinskiGL__Geometry__) */
