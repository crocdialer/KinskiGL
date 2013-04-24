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
#include "Renderer.h"
#include "geometry_types.h"

using namespace std;

namespace kinski { namespace gl {
    
    struct range_item_t
    {
        Object3DPtr object;
        float distance;
        range_item_t(){}
        range_item_t(Object3DPtr obj, float d):object(obj), distance(d){}
        bool operator <(const range_item_t &other) const {return distance < other.distance;}
    };
    
    void Scene::addObject(const Object3D::Ptr &theObject)
    {
        m_objects.push_back(theObject);
    }
    
    void Scene::removeObject(const Object3D::Ptr &theObject)
    {
        m_objects.remove(theObject);
    }
    
    void Scene::update(float time_delta)
    {
        list<Object3DPtr>::iterator objIt = m_objects.begin();
        for (; objIt != m_objects.end(); objIt++)
        {
            (*objIt)->update(time_delta);
        }
    }
    
    RenderBinPtr Scene::cull(const CameraPtr &theCamera) const
    {
        RenderBinPtr ret(new RenderBin(theCamera));
        glm::mat4 viewMatrix = theCamera->getViewMatrix();
        gl::Frustum frustum = theCamera->frustum();
        
        list<Object3DPtr>::const_iterator objIt = m_objects.begin();
        for (; objIt != m_objects.end(); objIt++)
        {
            if(const MeshPtr &theMesh = dynamic_pointer_cast<Mesh>(*objIt))
            {
                gl::AABB boundingBox = theMesh->geometry()->boundingBox();
                //gl::Sphere s(theMesh->position(), glm::length(boundingBox.halfExtents()));
                boundingBox.transform(theMesh->transform());
                
                if (frustum.intersect(boundingBox))
                {
                    RenderBin::item item;
                    item.mesh = theMesh;
                    item.transform = viewMatrix * theMesh->transform();
                    ret->items.push_back(item);
                }
            }
        }
        m_num_visible_objects = ret->items.size();
        return ret;
    }
    
    void Scene::render(const CameraPtr &theCamera) const
    {
        gl::Renderer r;
        r.render(cull(theCamera));
    }
    
    Object3DPtr Scene::pick(const Ray &ray, bool high_precision) const
    {
        Object3DPtr ret;
        std::list<range_item_t> clicked_items;
        list<Object3DPtr>::const_iterator objIt = m_objects.begin();
        for (; objIt != m_objects.end(); objIt++)
        {
            const Object3DPtr &theObj = *objIt;
            gl::OBB boundingBox (theObj->boundingBox(), theObj->transform());

            if (ray_intersection ray_hit = boundingBox.intersect(ray))
            {
                if(high_precision)
                {
                    if(gl::MeshPtr m = dynamic_pointer_cast<gl::Mesh>(theObj))
                    {
                        gl::Ray ray_in_object_space = ray.transform(glm::inverse(theObj->transform()));
                        const std::vector<glm::vec3>& vertices = m->geometry()->vertices();
                        std::vector<gl::Face3>::const_iterator it = m->geometry()->faces().begin();
                        for (; it != m->geometry()->faces().end(); ++it)
                        {
                            const Face3 &f = *it;
                            gl::Triangle t(vertices[f.a], vertices[f.b], vertices[f.c]);
                            
                            if(ray_triangle_intersection ray_tri_hit = t.intersect(ray_in_object_space))
                            {
                                clicked_items.push_back(range_item_t(theObj, ray_tri_hit.distance));
                                LOG_TRACE<<"hit distance: "<<ray_tri_hit.distance;
                            }
                        }
                    }
                }
                else
                {
                    clicked_items.push_back(range_item_t(theObj, ray_hit.distance));
                }
            }
        }
        if(!clicked_items.empty()){
            clicked_items.sort();
            ret = clicked_items.front().object;
            LOG_DEBUG<<"ray hit id "<<ret->getID()<<" ("<<clicked_items.size()<<" total)";
        }
        return ret;
    }
    
}}//namespace