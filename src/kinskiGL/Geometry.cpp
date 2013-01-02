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
    
    Geometry::Geometry():
    m_boundingBox(BoundingBox(glm::vec3(0), glm::vec3(0)))
    {
    
    }
    
    Geometry::~Geometry()
    {
  
    }
    
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
    
    void Geometry::appendNormals(const std::vector<glm::vec3> &theNormals)
    {
        m_normals.reserve(m_normals.size() + theNormals.size());
        m_normals.insert(m_normals.end(), theNormals.begin(), theNormals.end());
    }
    
    void Geometry::appendNormals(const glm::vec3 *theNormals, size_t numNormals)
    {
        m_normals.reserve(m_normals.size() + numNormals);
        m_normals.insert(m_normals.end(), theNormals, theNormals + numNormals);
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
    
    void Geometry::appendColors(const std::vector<glm::vec4> &theColors)
    {
        m_colors.reserve(m_colors.size() + theColors.size());
        m_colors.insert(m_colors.end(), theColors.begin(), theColors.end());
    }
    
    void Geometry::appendColors(const glm::vec4 *theColors, size_t numColors)
    {
        m_colors.reserve(m_colors.size() + numColors);
        m_colors.insert(m_colors.end(), theColors, theColors + numColors);
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
        m_boundingBox = BoundingBox(glm::vec3(numeric_limits<float>::infinity() ),
                                    glm::vec3(-numeric_limits<float>::infinity() ));
     
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
        if(m_normals.size() != m_vertices.size())
        {
            m_normals.clear();
            m_normals.reserve(m_vertices.size());
            
            for (int i = 0; i < m_vertices.size(); i++)
            {
                m_normals.push_back(glm::vec3(0));
            }
        }
        else
        {
            std::fill(m_normals.begin(), m_normals.end(), glm::vec3(0));
        }
        
        // iterate faces and sum normals for all vertices
        vector<Face3>::iterator faceIt = m_faces.begin();
        for (; faceIt != m_faces.end(); faceIt++)
        {
            const Face3 &face = *faceIt;
            
            m_normals[face.a] += face.normal;
            m_normals[face.b] += face.normal;
            m_normals[face.c] += face.normal;
        }
        
        // normalize vertexNormals
        vector<glm::vec3>::iterator normIt = m_normals.begin();
        for (; normIt != m_normals.end(); normIt++)
        {
            glm::vec3 &vertNormal = *normIt;
            vertNormal = glm::normalize(vertNormal);
        }
        
        // iterate faces again to fill in normals
        for (faceIt = m_faces.begin(); faceIt != m_faces.end(); faceIt++)
        {
            Face3 &face = *faceIt;
            face.vertexNormals[0] = m_normals[face.a];
            face.vertexNormals[1] = m_normals[face.b];
            face.vertexNormals[2] = m_normals[face.c];
        }
    }
    
    void Geometry::computeTangents()
    {
        if(m_texCoords.size() != m_vertices.size()) return;
        
        if(m_tangents.size() != m_vertices.size())
        {
            m_tangents.clear();
            m_tangents.reserve(m_vertices.size());
            
            for (int i = 0; i < m_vertices.size(); i++)
            {
                m_tangents.push_back(glm::vec3(0));
            }
        }
        
        vector<Face3>::iterator faceIt = m_faces.begin();
        for (; faceIt != m_faces.end(); faceIt++)
        {
            Face3 &face = *faceIt;
            
            const glm::vec3 &v0 = m_vertices[face.a], &v1 = m_vertices[face.b], &v2 = m_vertices[face.c];
            const glm::vec2 &t0 = m_texCoords[face.a], &t1 = m_texCoords[face.b], &t2 = m_texCoords[face.c];
            
            // calculate tangent vector
            float det = (t1.x - t0.x) * (t2.y - t0.y) - (t1.y - t0.y) * (t2.x - t0.x);
            glm::vec3 tangent = ( (t2.y - t0.y) * ( v1 - v0 ) - (t1.y - t0.y) * ( v2 - v0 ) ) / det;
            face.tangent = glm::normalize(tangent);
            
            m_tangents[face.a] = tangent;
            m_tangents[face.b] = tangent;
            m_tangents[face.c] = tangent;
        }
    }
    
    void Geometry::createGLBuffers()
    {
        m_vertexBuffer.setData(m_vertices);
        
        // insert normals
        if(hasNormals())
        {
            m_normalBuffer.setData(m_normals);
        }
        
        // insert normals
        if(hasTexCoords())
        {
            m_texCoordBuffer.setData(m_texCoords);
        }
        
        // insert tangents
        if(hasTangents())
        {
            m_tangentBuffer.setData(m_tangents);
        }
        
        // insert colors
        if(hasColors())
        {
            m_colorBuffer.setData(m_colors);
        }
        
        // insert bone indices and weights
        if(hasBones())
        {
            m_boneBuffer.setData(m_boneVertexData);
        }
        
        // index buffer
        m_indexBuffer = gl::Buffer(GL_ELEMENT_ARRAY_BUFFER, GL_STREAM_DRAW);
        
        m_indexBuffer.setData(NULL, 3 * m_faces.size() * sizeof(GLuint));
        
        GLuint *indexBuffer = (GLuint*) m_indexBuffer.map();
        
        // insert indices
        vector<gl::Face3>::const_iterator faceIt = m_faces.begin();
        for (; faceIt != m_faces.end(); ++faceIt)
        {
            const gl::Face3 &face = *faceIt;
            
            for (int i = 0; i < 3; i++)
            {
                // index
                *(indexBuffer++) = face.indices[i];
            }
        }
        
        m_indexBuffer.unmap();
    }
    
    void Geometry::updateAnimation(float time)
    {
        if(m_animation)
        {
            float t = fmod(time, m_animation->duration);
            
            m_boneMatrices.clear();
            buildBoneMatrices(t, m_rootBone, glm::mat4(), m_boneMatrices);
        }
    
    }
    
    void Geometry::buildBoneMatrices(float time, std::shared_ptr<Bone> bone,
                                     glm::mat4 parentTransform,
                                     std::vector<glm::mat4> &matrices)
    {
        const AnimationKeys &bonekeys = m_animation->boneKeys[bone];
        glm::mat4 boneTransform = bone->transform;
        
        bool boneHasKeys = false;
        
        // translation
        glm::mat4 translation;
        if(!bonekeys.positionkeys.empty())
        {
            boneHasKeys = true;
            int i = 0;
            for (; i < bonekeys.positionkeys.size() - 1; i++)
            {
                const Key<glm::vec3> &key = bonekeys.positionkeys[i + 1];
                if(key.time >= time)
                    break;
            }
            // i now holds the correct time index
            const Key<glm::vec3> &key1 = bonekeys.positionkeys[i],
            key2 = bonekeys.positionkeys[(i + 1) % bonekeys.positionkeys.size()];
            
            float startTime = key1.time;
            float endTime = key2.time < key1.time ? key2.time + m_animation->duration : key2.time;
            float frac = std::max( (time - startTime) / (endTime - startTime), 0.0f);
            glm::vec3 pos = glm::mix(key1.value, key2.value, frac);
            translation = glm::translate(translation, pos);
        }
        
        // rotation
        glm::mat4 rotation;
        if(!bonekeys.rotationkeys.empty())
        {
            boneHasKeys = true;
            int i = 0;
            for (; i < bonekeys.rotationkeys.size() - 1; i++)
            {
                const Key<glm::quat> &key = bonekeys.rotationkeys[i+1];
                if(key.time >= time)
                    break;
            }
            // i now holds the correct time index
            const Key<glm::quat> &key1 = bonekeys.rotationkeys[i],
            key2 = bonekeys.rotationkeys[(i + 1) % bonekeys.rotationkeys.size()];
            
            float startTime = key1.time;
            float endTime = key2.time < key1.time ? key2.time + m_animation->duration : key2.time;
            float frac = std::max( (time - startTime) / (endTime - startTime), 0.0f);
            
            // quaternion interpolation produces glitches
//            glm::quat interpolRot = glm::mix(key1.value, key2.value, frac);
//            rotation = glm::mat4_cast(interpolRot);
            
            glm::mat4 rot1 = glm::mat4_cast(key1.value), rot2 = glm::mat4_cast(key2.value);
            rotation = rot1 + frac * (rot2 - rot1);
        }
        
        // scale
        glm::mat4 scaleMatrix;
        if(!bonekeys.scalekeys.empty())
        {
            boneHasKeys = true;
            int i = 0;
            for (; i < bonekeys.scalekeys.size() - 1; i++)
            {
                const Key<glm::vec3> &key = bonekeys.scalekeys[i + 1];
                if(key.time >= time)
                    break;
            }
            // i now holds the correct time index
            const Key<glm::vec3> &key1 = bonekeys.scalekeys[i],
            key2 = bonekeys.scalekeys[(i + 1) % bonekeys.scalekeys.size()];
            
            float startTime = key1.time;
            float endTime = key2.time < key1.time ? key2.time + m_animation->duration : key2.time;
            float frac = std::max( (time - startTime) / (endTime - startTime), 0.0f);
            glm::vec3 scale = glm::mix(key1.value, key2.value, frac);
            scaleMatrix = glm::scale(scaleMatrix, scale);
        }
        
        if(boneHasKeys)
            boneTransform = translation * rotation * scaleMatrix;
        
        glm::mat4 globalTransform = parentTransform * boneTransform;
        bone->worldtransform = globalTransform;
        
        // add final transform
        glm::mat4 finalTransform = globalTransform * bone->offset;
        matrices.push_back(finalTransform);
        
        
        // recursion through all children
        list<shared_ptr<gl::Bone> >::iterator it = bone->children.begin();
        for (; it != bone->children.end(); ++it)
        {
            buildBoneMatrices(time, *it, globalTransform, matrices);
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
                appendNormal(normal);
                appendTextCoord( ix / (float)gridX, (gridZ - iz) / (float)gridZ);
                tangents().push_back(glm::vec3(0));
            }
        }
        
        std::vector<glm::vec3> vertNormals;
        vertNormals.push_back(normal);
        vertNormals.push_back(normal);
        vertNormals.push_back(normal);
        
        // create faces and texcoords
        for ( uint32_t iz = 0; iz < gridZ; iz ++ )
        {
            for ( uint32_t ix = 0; ix < gridX; ix ++ )
            {
                uint32_t a = ix + gridX1 * iz;
                uint32_t b = ix + gridX1 * ( iz + 1);
                uint32_t c = ( ix + 1 ) + gridX1 * ( iz + 1 );
                uint32_t d = ( ix + 1 ) + gridX1 * iz;
                
                Face3 f1(a, b, c), f2(c, d, a);
                f1.normal = normal;
                f2.normal = normal;
                
                f1.vertexNormals = vertNormals;
                f2.vertexNormals = vertNormals;
                
                appendFace(f1);
                appendFace(f2);
            }
        }
        
        computeTangents();
        createGLBuffers();
        computeBoundingBox();
    }
    
    Box::Box(glm::vec3 theHalfExtents)
    :Geometry()
    {
        glm::vec3 vertices[8] = {   glm::vec3(-theHalfExtents.x, -theHalfExtents.y, theHalfExtents.z),// bottom left front
                                    glm::vec3(theHalfExtents.x, -theHalfExtents.y, theHalfExtents.z),// bottom right front
                                    glm::vec3(theHalfExtents.x, -theHalfExtents.y, -theHalfExtents.z),// bottom right back
                                    glm::vec3(-theHalfExtents.x, -theHalfExtents.y, -theHalfExtents.z),// bottom left back
                                    glm::vec3(-theHalfExtents.x, theHalfExtents.y, theHalfExtents.z),// top left front
                                    glm::vec3(theHalfExtents.x, theHalfExtents.y, theHalfExtents.z),// top right front
                                    glm::vec3(theHalfExtents.x, theHalfExtents.y, -theHalfExtents.z),// top right back
                                    glm::vec3(-theHalfExtents.x, theHalfExtents.y, -theHalfExtents.z),// top left back
        };
        glm::vec4 colors[6] = { glm::vec4(1, 0, 0, 1), glm::vec4(0, 1, 0, 1), glm::vec4(0, 0 , 1, 1),
                                glm::vec4(1, 1, 0, 1), glm::vec4(0, 1, 1, 1), glm::vec4(1, 0 , 1, 1)};
        
        glm::vec2 texCoords[4] = {glm::vec2(0, 0), glm::vec2(1, 0), glm::vec2(1, 1), glm::vec2(0, 1)};
        
        glm::vec3 normals[6] = {glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec3(0, 0, -1),
                                glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0), glm::vec3(0, 1, 0)};
        
        std::vector<glm::vec3> vertexVec;
        std::vector<glm::vec4> colorVec;
        std::vector<glm::vec3> normalsVec;
        std::vector<glm::vec2> texCoordVec;
        
        //front - bottom left - 0
        vertexVec.push_back(vertices[0]); normalsVec.push_back(normals[0]);
        texCoordVec.push_back(texCoords[0]); colorVec.push_back(colors[0]);
        //front - bottom right - 1
        vertexVec.push_back(vertices[1]); normalsVec.push_back(normals[0]);
        texCoordVec.push_back(texCoords[1]); colorVec.push_back(colors[0]);
        //front - top right - 2
        vertexVec.push_back(vertices[5]); normalsVec.push_back(normals[0]);
        texCoordVec.push_back(texCoords[2]); colorVec.push_back(colors[0]);
        //front - top left - 3
        vertexVec.push_back(vertices[4]); normalsVec.push_back(normals[0]);
        texCoordVec.push_back(texCoords[3]); colorVec.push_back(colors[0]);
        //right - bottom left - 4
        vertexVec.push_back(vertices[1]); normalsVec.push_back(normals[1]);
        texCoordVec.push_back(texCoords[0]); colorVec.push_back(colors[1]);
        //right - bottom right - 5
        vertexVec.push_back(vertices[2]); normalsVec.push_back(normals[1]);
        texCoordVec.push_back(texCoords[1]); colorVec.push_back(colors[1]);
        //right - top right - 6 
        vertexVec.push_back(vertices[6]); normalsVec.push_back(normals[1]);
        texCoordVec.push_back(texCoords[2]); colorVec.push_back(colors[1]);
        //right - top left - 7
        vertexVec.push_back(vertices[5]); normalsVec.push_back(normals[1]);
        texCoordVec.push_back(texCoords[3]); colorVec.push_back(colors[1]);
        //back - bottom left - 8
        vertexVec.push_back(vertices[2]); normalsVec.push_back(normals[2]);
        texCoordVec.push_back(texCoords[0]); colorVec.push_back(colors[2]);
        //back - bottom right - 9
        vertexVec.push_back(vertices[3]); normalsVec.push_back(normals[2]);
        texCoordVec.push_back(texCoords[1]); colorVec.push_back(colors[2]);
        //back - top right - 10
        vertexVec.push_back(vertices[7]); normalsVec.push_back(normals[2]);
        texCoordVec.push_back(texCoords[2]); colorVec.push_back(colors[2]);
        //back - top left - 11
        vertexVec.push_back(vertices[6]); normalsVec.push_back(normals[2]);
        texCoordVec.push_back(texCoords[3]); colorVec.push_back(colors[2]);
        //left - bottom left - 12
        vertexVec.push_back(vertices[3]); normalsVec.push_back(normals[3]);
        texCoordVec.push_back(texCoords[0]); colorVec.push_back(colors[3]);
        //left - bottom right - 13
        vertexVec.push_back(vertices[0]); normalsVec.push_back(normals[3]);
        texCoordVec.push_back(texCoords[1]); colorVec.push_back(colors[3]);
        //left - top right - 14
        vertexVec.push_back(vertices[4]); normalsVec.push_back(normals[3]);
        texCoordVec.push_back(texCoords[2]); colorVec.push_back(colors[3]);
        //left - top left - 15
        vertexVec.push_back(vertices[7]); normalsVec.push_back(normals[3]);
        texCoordVec.push_back(texCoords[3]); colorVec.push_back(colors[3]);
        //bottom - bottom left - 16
        vertexVec.push_back(vertices[3]); normalsVec.push_back(normals[4]);
        texCoordVec.push_back(texCoords[0]); colorVec.push_back(colors[4]);
        //bottom - bottom right - 17
        vertexVec.push_back(vertices[2]); normalsVec.push_back(normals[4]);
        texCoordVec.push_back(texCoords[1]); colorVec.push_back(colors[4]);
        //bottom - top right - 18
        vertexVec.push_back(vertices[1]); normalsVec.push_back(normals[4]);
        texCoordVec.push_back(texCoords[2]); colorVec.push_back(colors[4]);
        //bottom - top left - 19
        vertexVec.push_back(vertices[0]); normalsVec.push_back(normals[4]);
        texCoordVec.push_back(texCoords[3]); colorVec.push_back(colors[4]);
        //top - bottom left - 20
        vertexVec.push_back(vertices[4]); normalsVec.push_back(normals[5]);
        texCoordVec.push_back(texCoords[0]); colorVec.push_back(colors[5]);
        //top - bottom right - 21
        vertexVec.push_back(vertices[5]); normalsVec.push_back(normals[5]);
        texCoordVec.push_back(texCoords[1]); colorVec.push_back(colors[5]);
        //top - top right - 22
        vertexVec.push_back(vertices[6]); normalsVec.push_back(normals[5]);
        texCoordVec.push_back(texCoords[2]); colorVec.push_back(colors[5]);
        //top - top left - 23
        vertexVec.push_back(vertices[7]); normalsVec.push_back(normals[5]);
        texCoordVec.push_back(texCoords[3]); colorVec.push_back(colors[5]);
        
        appendVertices(vertexVec);
        appendNormals(normalsVec);
        appendTextCoords(texCoordVec);
        appendColors(colorVec);
        
        for (int i = 0; i < 6; i++)
        {
            appendFace(i * 4 + 0, i * 4 + 1, i * 4 + 2);
            appendFace(i * 4 + 2, i * 4 + 3, i * 4 + 0);
        }
        
        computeTangents();
        createGLBuffers();
        computeBoundingBox();
    }
    
}}//namespace
