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
    m_boundingBox(BoundingBox(glm::vec3(0), glm::vec3(0))),
    m_interleavedBuffer(0),
    m_boneBuffer(0),
    m_colorBuffer(0),
    m_indexBuffer(0)
    {
    
    }
    
    Geometry::~Geometry()
    {
        if(m_interleavedBuffer)
            glDeleteBuffers(1, &m_interleavedBuffer);

        if(m_boneBuffer)
            glDeleteBuffers(1, &m_boneBuffer);

        if(m_colorBuffer)
            glDeleteBuffers(1, &m_colorBuffer);
        
        if(m_indexBuffer)
            glDeleteBuffers(1, &m_indexBuffer); 
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
    
    GLuint Geometry::numComponents()
    {
        //GL_T2F_N3F_V3F alignment in interleaved buffer
        return 11;
    }
    
    void Geometry::createGLBuffers()
    {
        if(!m_interleavedBuffer)
            glGenBuffers(1, &m_interleavedBuffer);
        
        if(!m_indexBuffer)
            glGenBuffers(1, &m_indexBuffer);
        
        uint32_t numFloats = numComponents();
        
        glBindBuffer(GL_ARRAY_BUFFER, m_interleavedBuffer);
        glBufferData(GL_ARRAY_BUFFER, numFloats * sizeof(GLfloat) * m_vertices.size(), NULL,
                     GL_STREAM_DRAW);//STREAM
        
        GLfloat *interleaved = (GLfloat*) GL_SUFFIX(glMapBuffer)(GL_ARRAY_BUFFER, GL_ENUM(GL_WRITE_ONLY));
        
        for (int i = 0; i < m_vertices.size(); i++)
        {
            // texCoords
            const glm::vec2 &texCoord = m_texCoords[i];
            interleaved[numFloats * i ] = texCoord.s;
            interleaved[numFloats * i + 1] = texCoord.t;
            
            // normals
            const glm::vec3 &normal = m_normals[i];
            interleaved[numFloats * i + 2] = normal.x;
            interleaved[numFloats * i + 3] = normal.y;
            interleaved[numFloats * i + 4] = normal.z;
            
            // vertices
            const glm::vec3 &vert = m_vertices[i];
            interleaved[numFloats * i + 5] = vert.x;
            interleaved[numFloats * i + 6] = vert.y;
            interleaved[numFloats * i + 7] = vert.z;
            
            // tangents
            const glm::vec3 &tangent = m_tangents[i];
            interleaved[numFloats * i + 8] = tangent.x;
            interleaved[numFloats * i + 9] = tangent.y;
            interleaved[numFloats * i + 10] = tangent.z;
        }
        
        GL_SUFFIX(glUnmapBuffer)(GL_ARRAY_BUFFER);
        
        // insert bone indices and weights
        if(hasColors())
        {
            if(!m_colorBuffer)
                glGenBuffers(1, &m_colorBuffer);
            
            glBindBuffer(GL_ARRAY_BUFFER, m_colorBuffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * m_vertices.size(),
                         &m_colors[0], GL_STREAM_DRAW);//STREAM
        }
        
        // insert bone indices and weights
        if(hasBones())
        {
            if(!m_boneBuffer)
                glGenBuffers(1, &m_boneBuffer);
            
            glBindBuffer(GL_ARRAY_BUFFER, m_boneBuffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(BoneVertexData) * m_vertices.size(),
                         &m_boneVertexData[0], GL_STREAM_DRAW);//STREAM
        }
        
        // index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * m_faces.size() * sizeof(GLuint), NULL,
                     GL_STREAM_DRAW );
        
        GLuint *indexBuffer = (GLuint*) GL_SUFFIX(glMapBuffer)(GL_ELEMENT_ARRAY_BUFFER,
                                                               GL_ENUM(GL_WRITE_ONLY));
        
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
        
        GL_SUFFIX(glUnmapBuffer)(GL_ELEMENT_ARRAY_BUFFER);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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
                {
                    break;
                }
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
                {
                    break;
                }
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
                {
                    break;
                }
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
        
        computeBoundingBox();
    }
    
    Box::Box(glm::vec3 theHalfExtents)
    :Geometry()
    {
        
    }
    
}}//namespace
