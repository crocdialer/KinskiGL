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

#include "KinskiGL.h"

namespace kinski { namespace gl {
    
    class KINSKI_API Scene
    {
    public:
        
        void addObject(const Object3DPtr &theObject);
        void removeObject(const Object3DPtr &theObject);
        
        void render(const CameraPtr &theCamera) const;
        void update(float time_delta);
        
        Object3DPtr pick(const Ray &ray, bool high_precision = false) const;
    
        inline const std::list<Object3DPtr>& objects() const {return m_objects;};
        inline std::list<Object3DPtr>& objects() {return m_objects;};
        uint32_t num_visible_objects() const {return m_num_visible_objects;};
        
    private:
        
        mutable uint32_t m_num_visible_objects;
        std::list<Object3DPtr> m_objects;
    };
    
}}//namespace

#endif /* defined(__kinskiGL__Scene__) */
