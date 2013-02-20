// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Object3D.h"

namespace kinski { namespace gl {
    
    uint32_t Object3D::s_idPool = 0;
    
    Object3D::Object3D(const glm::mat4 &theTransform):
    m_worldTransform(theTransform)
    {
        m_id = s_idPool++;
    }
    
    void Object3D::setRotation(const glm::quat &theRot)
    {
        glm::vec4 tmp = m_worldTransform[3];
        m_worldTransform = glm::mat4_cast(theRot);
        m_worldTransform[3] = tmp;
    }
    
    void Object3D::setRotation(const glm::mat3 &theRot)
    {
        glm::vec4 tmp = m_worldTransform[3];
        m_worldTransform = glm::mat4(theRot);
        m_worldTransform[3] = tmp;
    }
    
    glm::quat Object3D::rotation() const
    {
        return glm::quat_cast(m_worldTransform);
    }
    
}}//namespace
