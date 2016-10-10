// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include <algorithm>
#include "gl/geometry_types.hpp"
#include "Object3D.hpp"
#include "Visitor.hpp"

namespace kinski { namespace gl {
    
    uint32_t Object3D::s_id_pool = 0;
    
    // static factory
    Object3DPtr Object3D::create(){ return Object3DPtr(new Object3D()); }
    
    Object3D::Object3D():
    m_id(s_id_pool++),
    m_name("Object3D " + to_string(m_id)),
    m_enabled(true),
    m_billboard(false)
    {

    }
    
    bool Object3D::has_tag(const std::string& the_tag) const
    {
        return m_tags.find(the_tag) != m_tags.end();
    }
    
    void Object3D::set_rotation(const glm::quat &theRot)
    {
        glm::vec3 pos_tmp(position()), scale_tmp(scale());
        m_transform = glm::mat4_cast(theRot);
        set_position(pos_tmp);
        set_scale(scale_tmp);
    }
    
    void Object3D::set_rotation(const glm::mat3 &theRot)
    {
        glm::vec3 pos_tmp(position()), scale_tmp(scale());
        m_transform = glm::mat4(theRot);
        set_position(pos_tmp);
        set_scale(scale_tmp);
    }
    
    void Object3D::set_rotation(float pitch, float yaw, float roll)
    {
        glm::vec3 pos_tmp(position()), scale_tmp(scale());
        m_transform = glm::mat4_cast(glm::quat(glm::vec3(glm::radians(pitch),
                                                         glm::radians(yaw),
                                                         glm::radians(roll))));
        set_position(pos_tmp);
        set_scale(scale_tmp);
    }
    
    glm::quat Object3D::rotation() const
    {
        return glm::normalize(glm::quat_cast(m_transform));
    }
    
    void Object3D::set_look_at(const glm::vec3 &theLookAt, const glm::vec3 &theUp)
    {
        set_transform(glm::inverse(glm::lookAt(position(), theLookAt, theUp)) * glm::scale(mat4(), scale()));
    }
    
    void Object3D::set_look_at(const Object3DPtr &theLookAt)
    {
        set_look_at(-theLookAt->position() + position(), theLookAt->up());
    }
    
    void Object3D::set_scale(const glm::vec3 &s)
    {
        glm::vec3 scale_vec = s / scale();
        m_transform = glm::scale(m_transform, scale_vec);
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

            // prevent multiple insertions
            if(std::find(m_children.begin(), m_children.end(), the_child) == m_children.end())
            {
                m_children.push_back(the_child);
            }
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
        mat4 global_trans = global_transform();
        ret.transform(global_trans);
        
        for (auto &c :children())
        {
            ret += c->boundingBox();
        }
        return ret;
    }
    
    gl::OBB Object3D::obb() const
    {
        gl::OBB ret(boundingBox(), mat4());
        return ret;
    }
    
    void Object3D::add_tag(const std::string& the_tag, bool recursive)
    {
        m_tags.insert(the_tag);
        
        if(recursive)
        {
            for (auto &c : children()){ c->add_tag(the_tag, recursive); }
        }

    }
    
    void Object3D::remove_tag(const std::string& the_tag, bool recursive)
    {
        m_tags.erase(the_tag);
        
        if(recursive)
        {
            for (auto &c : children()){ c->remove_tag(the_tag, recursive); }
        }
    }
    
    void Object3D::accept(Visitor &theVisitor)
    {
        theVisitor.visit(*this);
    }

}}//namespace
