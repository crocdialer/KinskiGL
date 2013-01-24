//
//  Scene.cpp
//  kinskiGL
//
//  Created by Fabian on 11/2/12.
//
//

#include "Scene.h"
#include "Mesh.h"
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
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
        gl::Frustum frustum;
        
        ScopedMatrixPush proj(gl::PROJECTION_MATRIX), mod(gl::MODEL_VIEW_MATRIX);
        
        gl::loadMatrix(gl::PROJECTION_MATRIX, theCamera->getProjectionMatrix());
        
        glm::mat4 viewMatrix = theCamera->getViewMatrix();
        
        if(const PerspectiveCamera::Ptr &cam = dynamic_pointer_cast<PerspectiveCamera>(theCamera))
        {
            frustum = gl::Frustum(cam->transform(), cam->fov(), cam->near(),
                                          cam->far());
        }
        else if(const OrthographicCamera::Ptr &cam = dynamic_pointer_cast<OrthographicCamera>(theCamera))
        {
            frustum = gl::Frustum(cam->transform(), cam->left(), cam->right(), cam->bottom(),
                                  cam->top(), cam->near(), cam->far());
        }
        
        //map<boost::tuple<Geometry::Ptr, Material::Ptr>, list<glm::mat4> > transformMap;
        int counter = 0;
        
        list<Object3D::Ptr>::const_iterator objIt = m_objects.begin();
        for (; objIt != m_objects.end(); objIt++)
        {
            gl::loadMatrix(gl::MODEL_VIEW_MATRIX, viewMatrix * (*objIt)->transform());
            
            if(const Mesh::Ptr &theMesh = dynamic_pointer_cast<Mesh>(*objIt))
            {
//                gl::AABB boundingBox;
//                boundingBox.min = theMesh->geometry()->boundingBox().min;
//                boundingBox.max = theMesh->geometry()->boundingBox().max;
                
                gl::Sphere boundingSphere (theMesh->position(),
                                           std::max(theMesh->geometry()->boundingBox().min.length(),
                                                    theMesh->geometry()->boundingBox().max.length()));
                
                if (frustum.intersect(boundingSphere))
                {
                    gl::drawMesh(theMesh);
                    counter++;
                }
//                transformMap[boost::tuple<Geometry::Ptr, Material::Ptr>(theMesh->geometry(),
//                                                                        theMesh->material())].push_back(theMesh->transform());
                
            }

        }
        //LOG_INFO<<counter;
    }
    
    bool isVisible(const Camera::Ptr &theCamera, const Object3D::Ptr theObject)
    {
        return false;
    }
    
}}//namespace