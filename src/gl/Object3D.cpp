// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include <algorithm>
#include "gl/geometry_types.h"
#include "Object3D.h"
#include "Visitor.h"

namespace kinski { namespace gl {
    
    uint32_t Object3D::s_idPool = 0;
    
    // static factory
    Object3DPtr Object3D::create(){ return Object3DPtr(new Object3D()); }
    
    Object3D::Object3D():
    m_id(s_idPool++),
    m_name("Object3D " + as_string(m_id)),
    m_enabled(true),
    m_billboard(false)
    {

    }
    
    void Object3D::setRotation(const glm::quat &theRot)
    {
        glm::vec3 pos_tmp(position()), scale_tmp(scale());
        m_transform = glm::mat4_cast(theRot);
        setPosition(pos_tmp);
        setScale(scale_tmp);
    }
    
    void Object3D::setRotation(const glm::mat3 &theRot)
    {
        glm::vec3 pos_tmp(position()), scale_tmp(scale());
        m_transform = glm::mat4(theRot);
        setPosition(pos_tmp);
        setScale(scale_tmp);
    }
    
    void Object3D::setRotation(float pitch, float yaw, float roll)
    {
        glm::vec3 pos_tmp(position()), scale_tmp(scale());
        m_transform = glm::mat4_cast(glm::quat(glm::vec3(glm::radians(pitch),
                                                         glm::radians(yaw),
                                                         glm::radians(roll))));
        setPosition(pos_tmp);
        setScale(scale_tmp);
    }
    
    glm::quat Object3D::rotation() const
    {
        return glm::normalize(glm::quat_cast(m_transform));
    }
    
    void Object3D::setLookAt(const glm::vec3 &theLookAt, const glm::vec3 &theUp)
    {
        setTransform( glm::inverse(glm::lookAt(position(), theLookAt, theUp)) );
    }
    
    void Object3D::setLookAt(const Object3DPtr &theLookAt)
    {
        setLookAt(-theLookAt->position() + position(), theLookAt->up());
    }
    
    void Object3D::setScale(const glm::vec3 &s)
    {
        glm::vec3 scale_vec = s / scale();
        if(std::abs(s.x) > 0.f &&
           std::abs(s.y) > 0.f &&
           std::abs(s.z) > 0.f)
        {
            m_transform = glm::scale(m_transform, scale_vec);
        }
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
    
    glm::vec3 Object3D::global_position() const
    {
        glm::mat4 global_trans = global_transform();
        return global_trans[3].xyz();
    }
    
    glm::quat Object3D::global_rotation() const
    {
        glm::mat4 global_trans = global_transform();
        return glm::normalize(glm::quat_cast(global_trans));
    }
    
    glm::vec3 Object3D::global_scale() const
    {
        glm::mat4 global_trans = global_transform();
        return glm::vec3(glm::length(global_trans[0]),
                         glm::length(global_trans[1]),
                         glm::length(global_trans[2]));
    }
    
    void Object3D::set_global_transform(const glm::mat4 &transform)
    {
        glm::mat4 global_trans_inv = glm::inverse(global_transform());
        m_transform = global_trans_inv * transform;
    }
    
    void Object3D::set_global_position(const glm::vec3 &position)
    {
        glm::mat4 global_trans_inv = glm::inverse(global_transform());
        m_transform = global_trans_inv * glm::translate(glm::mat4(), position);
    }
    
    void Object3D::set_global_rotation(const glm::quat &rotation)
    {
        glm::mat4 global_trans_inv = glm::inverse(global_transform());
        m_transform = global_trans_inv * glm::mat4_cast(rotation);
    }
    
    void Object3D::set_global_scale(const glm::vec3 &the_scale)
    {
        glm::mat4 global_trans_inv = glm::inverse(global_transform());
        m_transform = global_trans_inv * glm::scale(glm::mat4(), the_scale);
    }
    
    void Object3D::set_parent(const Object3DPtr &the_parent)
    {
        // detach object from former parent
        if(Object3DPtr p = parent())
        {
            p->remove_child(shared_from_this());
        }

        if(the_parent)
        {
            the_parent->add_child(shared_from_this());
        }
        else
        {
            m_parent.reset();
        }
    }
    
    void Object3D::add_child(const Object3DPtr &the_child)
    {
        if(the_child)
        {
            the_child->set_parent(Object3DPtr());
            the_child->m_parent = shared_from_this();
        }
        // prevent multiple insertions
        if(std::find(m_children.begin(), m_children.end(), the_child) == m_children.end())
        {
            m_children.push_back(the_child);
        }
    }
    
    void Object3D::remove_child(const Object3DPtr &the_child, bool recursive)
    {
        std::list<Object3DPtr>::iterator it = std::find(m_children.begin(), m_children.end(), the_child);
        if(it != m_children.end())
        {
            m_children.erase(it);
            if(the_child){the_child->set_parent(Object3DPtr());}
        }
        // not a direct descendant, go on recursive if requested
        else if(recursive)
        {
            for(auto &c : children())
            {
                c->remove_child(the_child, recursive);
            }
        }
    }
    
    AABB Object3D::boundingBox() const
    {
        AABB ret;
        std::list<Object3DPtr>::const_iterator it = m_children.begin();
        for (; it != m_children.end(); ++it)
        {
            ret += (*it)->boundingBox().transform((*it)->global_transform());
        }
        return ret;
    }
    
    void Object3D::accept(Visitor &theVisitor)
    {
        theVisitor.visit(*this);
    }

}}//namespace
