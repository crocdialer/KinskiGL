// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Scene.h"
#include "Mesh.h"
#include "geometry_types.h"

using namespace std;

namespace kinski { namespace gl {
    
    void Scene::addObject(const Object3D::Ptr &theObject)
    {
        m_objects.push_back(theObject);
    }
    
    void Scene::removeObject(const Object3D::Ptr &theObject)
    {
        m_objects.remove(theObject);
    }
    
    void Scene::update(float timestep)
    {
        
    }
    
    void Scene::render(const Camera::Ptr &theCamera) const
    {
        ScopedMatrixPush proj(gl::PROJECTION_MATRIX), mod(gl::MODEL_VIEW_MATRIX);
        gl::loadMatrix(gl::PROJECTION_MATRIX, theCamera->getProjectionMatrix());
        
        glm::mat4 viewMatrix = theCamera->getViewMatrix();
        
        gl::Frustum frustum = theCamera->frustum();
        
        map<std::pair<Geometry::Ptr, Material::Ptr>, list<Mesh::Ptr> > meshMap;
        m_num_visible_objects = 0;
        
        list<Object3D::Ptr>::const_iterator objIt = m_objects.begin();
        for (; objIt != m_objects.end(); objIt++)
        {
            gl::loadMatrix(gl::MODEL_VIEW_MATRIX, viewMatrix * (*objIt)->transform());
            
            if(const Mesh::Ptr &theMesh = dynamic_pointer_cast<Mesh>(*objIt))
            {
                gl::AABB boundingBox;
                boundingBox.min = theMesh->geometry()->boundingBox().min;
                boundingBox.max = theMesh->geometry()->boundingBox().max;
                boundingBox.transform(theMesh->transform());
                
                if (frustum.intersect(boundingBox))
                {
                    //gl::drawMesh(theMesh);
                    meshMap[std::pair<Geometry::Ptr, Material::Ptr>(theMesh->geometry(),
                                                                    theMesh->material())].push_back(theMesh);
                    
                    m_num_visible_objects++;
                }
                
            }

        }
        
        map<std::pair<Geometry::Ptr, Material::Ptr>, list<Mesh::Ptr> >::const_iterator it = meshMap.begin();
        
        for (; it != meshMap.end(); ++it)
        {
            const list<Mesh::Ptr>& meshList = it->second;
            Mesh::Ptr m = meshList.front();
            
            if(m->geometry()->hasBones())
            {
                m->material()->uniform("u_bones", m->geometry()->boneMatrices());
            }
            
            m->material()->apply();
            
#ifndef KINSKI_NO_VAO
            GL_SUFFIX(glBindVertexArray)(m->vertexArray());
#else
            m->bindVertexPointers();
#endif
            
            list<Mesh::Ptr>::const_iterator transformIt = meshList.begin();
            for (; transformIt != meshList.end(); ++transformIt)
            {
                m = *transformIt;
                
                glm::mat4 modelView = viewMatrix * m->transform();
                
                m->material()->shader().uniform("u_modelViewMatrix", modelView);
                
                m->material()->shader().uniform("u_normalMatrix",
                                                glm::inverseTranspose( glm::mat3(modelView) ));
                
                m->material()->shader().uniform("u_modelViewProjectionMatrix",
                                                theCamera->getProjectionMatrix() * modelView);
                
                if(m->geometry()->hasIndices())
                {
                    glDrawElements(m->geometry()->primitiveType(),
                                   m->geometry()->indices().size(), m->geometry()->indexType(),
                                   BUFFER_OFFSET(0));
                }
                else
                {
                    glDrawArrays(m->geometry()->primitiveType(), 0,
                                 m->geometry()->vertices().size());
                }
            }
            
#ifndef KINSKI_NO_VAO
            GL_SUFFIX(glBindVertexArray)(0);
#endif
            
        }
        
    }
    
    bool isVisible(const Camera::Ptr &theCamera, const Object3D::Ptr theObject)
    {
        return false;
    }
    
}}//namespace