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
        void update(float timestep);
        
        inline const std::list<Object3D::Ptr>& objects() const {return m_objects;};
        inline std::list<Object3D::Ptr>& objects() {return m_objects;};
        
    private:
        
        bool isVisible(const Camera::Ptr &theCamera, const Object3D::Ptr theObject);
        
        std::list<Object3D::Ptr> m_objects;
    };
    
}}//namespace

#endif /* defined(__kinskiGL__Scene__) */
