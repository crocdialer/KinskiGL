// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

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
        
        uint32_t num_visible_objects() const {return m_num_visible_objects;};
        
    private:
        
        mutable uint32_t m_num_visible_objects;
        
        std::list<Object3D::Ptr> m_objects;
    };
    
}}//namespace

#endif /* defined(__kinskiGL__Scene__) */
