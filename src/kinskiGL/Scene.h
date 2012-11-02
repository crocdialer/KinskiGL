//
//  Scene.h
//  kinskiGL
//
//  Created by Fabian on 11/2/12.
//
//

#ifndef __kinskiGL__Scene__
#define __kinskiGL__Scene__

#include "Camera.h"

namespace kinski { namespace gl {
    
    class Scene
    {
    public:
        
        void addObject(const Object3D::Ptr &theObject);
        void removeObject(const Object3D::Ptr &theObject);
        
        void render(const Camera::Ptr &theCamera) const;
        
    private:
        
        std::list<Object3D::Ptr> m_objects;
    };
    
}}//namespace

#endif /* defined(__kinskiGL__Scene__) */
