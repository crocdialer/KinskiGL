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
#include "Camera.h"
#include "geometry_types.h"

using namespace std;

namespace kinski { namespace gl {
    
    struct range_item_t
    {
        Object3DPtr object;
        float distance;
        
        range_item_t(){}
        range_item_t(Object3DPtr obj, float d):object(obj), distance(d){}
        bool operator ()(const range_item_t &lhs,const range_item_t &rhs)
        { return lhs.distance < rhs.distance; }
    };
    
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
    
    void Scene::render(const CameraPtr &theCamera) const
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
            if(const Mesh::Ptr &theMesh = dynamic_pointer_cast<Mesh>(*objIt))
            {
                gl::AABB boundingBox = theMesh->geometry()->boundingBox();
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
        
        map<std::pair<GeometryPtr, MaterialPtr>, list<MeshPtr> >::const_iterator it = meshMap.begin();
        
        for (; it != meshMap.end(); ++it)
        {
            const list<Mesh::Ptr>& meshList = it->second;
            MeshPtr m = meshList.front();
            
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
    
    Object3D::Ptr Scene::pick(const CameraPtr &theCamera, uint32_t x, uint32_t y) const
    {
        Object3DPtr ret;

        glm::vec3 cam_pos = theCamera->position();
        glm::vec3 lookAt = theCamera->lookAt(),
        side = theCamera->side(), up = theCamera->up();
        
        float near = theCamera->near();
        
        // bring click_pos to range -1, 1
        glm::vec2 offset (gl::windowDimension() / 2.0f);
        glm::vec2 click_2D(x, y);
        click_2D -= offset;
        click_2D /= offset;
        click_2D.y = - click_2D.y;
        
        glm::vec3 click_world_pos;
        
        if(PerspectiveCamera::Ptr cam = dynamic_pointer_cast<PerspectiveCamera>(theCamera) )
        {
            // convert fovy to radians
            float rad = glm::radians(cam->fov());
            float vLength = tan( rad / 2) * near;
            float hLength = vLength * cam->aspectRatio();
            
            click_world_pos = cam_pos + lookAt * near
                + side * hLength * click_2D.x
                + up * vLength * click_2D.y;
            
        }else if (OrthographicCamera::Ptr cam = dynamic_pointer_cast<OrthographicCamera>(theCamera))
        {
            click_world_pos = cam_pos + lookAt * near + side * click_2D.x + up  * click_2D.y;
        }
        
        Ray ray(click_world_pos, click_world_pos - cam_pos);
        
        LOG_DEBUG<<"clicked_world: ("<<click_world_pos.x<<",  "<<click_world_pos.y<<",  "<<click_world_pos.z<<")";
        
        //LOG_INFO<<"ray_dir: "<<ray.direction.x<<"  "<<ray.direction.y<<"  "<<ray.direction.z;
        std::list<range_item_t> clicked_items;
        list<Object3D::Ptr>::const_iterator objIt = m_objects.begin();
        for (; objIt != m_objects.end(); objIt++)
        {
            if(const Mesh::Ptr &theMesh = dynamic_pointer_cast<Mesh>(*objIt))
            {
                gl::AABB boundingBox = theMesh->geometry()->boundingBox();
                boundingBox.transform(theMesh->transform());
                //gl::Sphere s(theMesh->position(), boundingBox.halfExtents().length());
                
                if (ray_intersection ray_hit = boundingBox.intersect(ray))
                {
                    //LOG_INFO<<"ray hit object (id: "<<theMesh->getID()<<")";
                    clicked_items.push_back(range_item_t(theMesh, ray_hit.distance));
                }
            }
        }
        
        LOG_INFO<<"ray hit "<<clicked_items.size()<<" objects";
        if(!clicked_items.empty()){
            clicked_items.sort(range_item_t());
            ret = clicked_items.front().object;
        }
        
        return ret;
    }
    
}}//namespace