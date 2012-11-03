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
        glm::mat4 projectionMatrix = theCamera->getProjectionMatrix();
        
        glm::mat4 viewMatrix = theCamera->getViewMatrix();
        
        list<Object3D::Ptr>::const_iterator objIt = m_objects.begin();
        for (; objIt != m_objects.end(); objIt++)
        {
            glm::mat4 modelViewMatrix = viewMatrix * (*objIt)->getTransform();
            
            if(const Mesh::Ptr &theMesh = dynamic_pointer_cast<Mesh>(*objIt))
            {
                theMesh->getMaterial()->uniform("u_modelViewMatrix", modelViewMatrix);
                
                theMesh->getMaterial()->uniform("u_normalMatrix",
                                                glm::inverseTranspose(glm::mat3(modelViewMatrix)) );
                
                theMesh->getMaterial()->uniform("u_modelViewProjectionMatrix",
                                                projectionMatrix * modelViewMatrix);
                
                theMesh->getMaterial()->apply();
                
                glBindVertexArray(theMesh->getVertexArray());
                glDrawElements(GL_TRIANGLES, 3 * theMesh->getGeometry()->getFaces().size(),
                               GL_UNSIGNED_INT, BUFFER_OFFSET(0));
                glBindVertexArray(0);
            }

        }
    }
    
}}//namespace