// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef __kinskiGL__Object3D__
#define __kinskiGL__Object3D__

#include "KinskiGL.h"

namespace kinski { namespace gl {

    class Object3D
    {
    public:
        
        typedef std::shared_ptr<Object3D> Ptr;
        
        Object3D(const glm::mat4 &theTransform = glm::mat4());
        virtual ~Object3D(){};
        
        uint32_t getID() const { return m_id; };
        inline void setPosition(const glm::vec3 &thePos) { m_worldTransform[3].xyz() = thePos; };
        
        inline glm::vec3 position() const { return m_worldTransform[3].xyz(); }
        inline glm::vec3 lookAt() const { return glm::normalize(-m_worldTransform[2].xyz()); }
        inline glm::vec3 side() const { return glm::normalize(m_worldTransform[0].xyz()); }
        inline glm::vec3 up() const { return glm::normalize(m_worldTransform[1].xyz()); }
        
        void setRotation(const glm::quat &theRot);
        void setRotation(const glm::mat3 &theRot);
        glm::quat rotation() const;
        
        inline void setTransform(const glm::mat4 &theTrans){ m_worldTransform = theTrans; };
        inline glm::mat4& transform() { return m_worldTransform; };
        inline const glm::mat4& transform() const { return m_worldTransform; };
        
    private:
        
        static uint32_t s_idPool;
        
        uint32_t m_id;
        glm::mat4 m_worldTransform;
    };

}}//namespace

#endif /* defined(__kinskiGL__Object3D__) */
