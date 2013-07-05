// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "kinskiGL/geometry_types.h"
#include "Object3D.h"

namespace kinski { namespace gl {
    
    uint32_t Object3D::s_idPool = 0;
    
    Object3D::Object3D():
    m_enabled(true),
    m_billboard(false)
    {
        m_id = s_idPool++;
    }
    
    void Object3D::setRotation(const glm::quat &theRot)
    {
        glm::vec4 tmp = m_transform[3];
        m_transform = glm::mat4_cast(theRot);
        m_transform[3] = tmp;
    }
    
    void Object3D::setRotation(const glm::mat3 &theRot)
    {
        glm::vec4 tmp = m_transform[3];
        m_transform = glm::mat4(theRot);
        m_transform[3] = tmp;
    }
    
    glm::quat Object3D::rotation() const
    {
        return glm::normalize(glm::quat_cast(m_transform));
    }
    
    void Object3D::setLookAt(const glm::vec3 &theLookAt, const glm::vec3 &theUp)
    {
        setTransform( glm::inverse(glm::lookAt(position(), theLookAt, theUp)) );
    }
    
    glm::mat4 Object3D::global_transform() const
    {
        glm::mat4 ret = transform();
        Object3DPtr ancestor = parent();
        while (ancestor)
        {
            ret = ancestor->transform() * ret;
            ancestor = ancestor->parent();
        }
        return ret;
    }
    
    void Object3D::add_child(const Object3DPtr &the_child)
    {
        the_child->set_parent(shared_from_this());
        m_children.push_back(the_child);
    }
    
    AABB Object3D::boundingBox() const
    {
        AABB ret;
        std::list<Object3DPtr>::const_iterator it = m_children.begin();
        for (; it != m_children.end(); ++it)
        {
            ret += (*it)->boundingBox();
        }
        return ret;
    }
    
    void Object3D::accept(Visitor &theVisitor)
    {
        theVisitor.visit(*this);
    }

}}//namespace
