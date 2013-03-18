// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Geometry.h"

#ifdef KINSKI_GLES
typedef GLushort index_type;
#else
typedef GLuint index_type;
#endif

using namespace std;

namespace kinski{ namespace gl{
    
    Geometry::Geometry():
    m_primitiveType(GL_TRIANGLES)
    {
    
    }
    
    Geometry::~Geometry()
    {
  
    }

    void Geometry::computeBoundingBox()
    {
        m_boundingBox = gl::calculateAABB(m_vertices);
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
        }
    }
    
    void Geometry::computeVertexNormals()
    {
        if(m_faces.empty()) return;
        
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
    }
    
    void Geometry::computeTangents()
    {
        if(m_faces.empty()) return;

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
            tangent = glm::normalize(tangent);
            m_tangents[face.a] = tangent;
            m_tangents[face.b] = tangent;
            m_tangents[face.c] = tangent;
        }
    }
    
    void Geometry::createGLBuffers()
    {
        m_vertexBuffer.setData(m_vertices);
        KINSKI_CHECK_GL_ERRORS();
        
        // insert normals
        if(hasNormals())
        {
            m_normalBuffer.setData(m_normals);
            KINSKI_CHECK_GL_ERRORS();
        }
        
        // insert normals
        if(hasTexCoords())
        {
            m_texCoordBuffer.setData(m_texCoords);
            KINSKI_CHECK_GL_ERRORS();
        }
        
        // insert tangents
        if(hasTangents())
        {
            m_tangentBuffer.setData(m_tangents);
            KINSKI_CHECK_GL_ERRORS();
        }
        
        // insert colors
        if(hasColors())
        {
            m_colorBuffer.setData(m_colors);
            KINSKI_CHECK_GL_ERRORS();
        }
        
        // insert bone indices and weights
        if(hasBones())
        {
            m_boneBuffer.setData(m_boneVertexData);
            KINSKI_CHECK_GL_ERRORS();
        }
        
        if(hasIndices())
        {
            // index buffer
            m_indexBuffer = gl::Buffer(GL_ELEMENT_ARRAY_BUFFER, GL_DYNAMIC_DRAW);
            
            m_indexBuffer.setData(NULL, m_indices.size() * sizeof(index_type));
            KINSKI_CHECK_GL_ERRORS();
            
            index_type *indexPtr = (index_type*) m_indexBuffer.map();
            KINSKI_CHECK_GL_ERRORS();
            
            // insert indices
            vector<uint32_t>::const_iterator indexIt = m_indices.begin();
            for (; indexIt != m_indices.end(); ++indexIt)
            {
                *indexPtr++ = *indexIt;
            }
            
            m_indexBuffer.unmap();
            KINSKI_CHECK_GL_ERRORS();
        }
    }
    
    GLenum Geometry::indexType()
    {
#if defined(KINSKI_GLES)
        GLenum ret = GL_UNSIGNED_SHORT;
#else
        GLenum ret = GL_UNSIGNED_INT;
#endif
        return ret;
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
    
    Geometry::Ptr createPlane(float width, float height,
                              uint32_t numSegments_W , uint32_t numSegments_H)
    {
        Geometry::Ptr geom (new Geometry);
        
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
                
                geom->appendVertex( glm::vec3( x, - y, 0) );
                geom->appendNormal(normal);
                geom->appendTextCoord( ix / (float)gridX, (gridZ - iz) / (float)gridZ);
                geom->tangents().push_back(glm::vec3(0));
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
                
                geom->appendFace(f1);
                geom->appendFace(f2);
            }
        }
        
        geom->computeTangents();
        geom->createGLBuffers();
        geom->computeBoundingBox();
        
        return geom;
    }
    
    Geometry::Ptr createBox(const glm::vec3 &theHalfExtents)
    {
        Geometry::Ptr geom (new Geometry);
        
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
        
        geom->appendVertices(vertexVec);
        geom->appendNormals(normalsVec);
        geom->appendTextCoords(texCoordVec);
        geom->appendColors(colorVec);
        
        for (int i = 0; i < 6; i++)
        {
            geom->appendFace(i * 4 + 0, i * 4 + 1, i * 4 + 2);
            geom->appendFace(i * 4 + 2, i * 4 + 3, i * 4 + 0);
        }
        geom->computeTangents();
        geom->createGLBuffers();
        geom->computeBoundingBox();
        return geom;
    }
    
    Geometry::Ptr createSphere(float radius, int numSlices)
    {
        uint32_t rings = numSlices, sectors = numSlices;
        Geometry::Ptr geom (new Geometry);
        float const R = 1./(float)(rings-1);
        float const S = 1./(float)(sectors-1);
        int r, s;
        
        geom->vertices().resize(rings * sectors);
        geom->normals().resize(rings * sectors);
        geom->texCoords().resize(rings * sectors);
        std::vector<glm::vec3>::iterator v = geom->vertices().begin();
        std::vector<glm::vec3>::iterator n = geom->normals().begin();
        std::vector<glm::vec2>::iterator t = geom->texCoords().begin();
        for(r = 0; r < rings; r++)
            for(s = 0; s < sectors; s++, ++v, ++n, ++t)
            {
                float const y = sin( -M_PI_2 + M_PI * r * R );
                float const x = cos(2*M_PI * s * S) * sin( M_PI * r * R );
                float const z = sin(2*M_PI * s * S) * sin( M_PI * r * R );
                
                *t = glm::vec2(s * S, r * R);
                *v = glm::vec3(x, y, z) * radius;
                *n = glm::vec3(x, y, z);
            }

        for(r = 0; r < rings-1; r++)
            for(s = 0; s < sectors-1; s++)
            {
                geom->appendFace(r * sectors + s, (r+1) * sectors + (s+1), r * sectors + (s+1));
                geom->appendFace(r * sectors + s, (r+1) * sectors + s, (r+1) * sectors + (s+1));
            }
        
        geom->computeTangents();
        geom->createGLBuffers();
        geom->computeBoundingBox();
        return geom;
    }

}}//namespace
