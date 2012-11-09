//
//  Scene.cpp
//  kinskiGL
//
//  Created by Fabian on 11/2/12.
//
//

#include "Scene.h"
#include "Mesh.h"

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
    
    void Scene::render(const Camera::Ptr &theCamera) const
    {
        ScopedMatrixPush proj(gl::PROJECTION_MATRIX), mod(gl::MODEL_VIEW_MATRIX);
        
        gl::loadMatrix(gl::PROJECTION_MATRIX, theCamera->getProjectionMatrix());
        
        glm::mat4 viewMatrix = theCamera->getViewMatrix();
        
        list<Object3D::Ptr>::const_iterator objIt = m_objects.begin();
        for (; objIt != m_objects.end(); objIt++)
        {
            gl::loadMatrix(gl::MODEL_VIEW_MATRIX, viewMatrix * (*objIt)->getTransform());
            
            if(const Mesh::Ptr &theMesh = dynamic_pointer_cast<Mesh>(*objIt))
            {
                gl::drawMesh(theMesh);
            }

        }
    }
    
}}//namespace