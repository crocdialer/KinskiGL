//
//  Scene.cpp
//  kinskiGL
//
//  Created by Fabian on 11/2/12.
//
//

#include "Scene.h"

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
    
    }
    
}}//namespace