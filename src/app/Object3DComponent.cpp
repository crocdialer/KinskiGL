// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  Object3DComponent.cpp
//
//  Created by Fabian on 6/28/13.

#include "Object3DComponent.hpp"
#include "gl/Mesh.hpp"

using namespace crocore;

namespace kinski {
Object3DComponent::Object3DComponent() :
        m_object_index(RangedProperty<int>::create("index", -1, -1, 256)),
        m_enabled(Property_<bool>::create("enabled", true)),
        m_position_x(Property_<float>::create("position X", 0)),
        m_position_y(Property_<float>::create("position Y", 0)),
        m_position_z(Property_<float>::create("position Z", 0)),
        m_scale(Property_<glm::vec3>::create("scale", glm::vec3(1))),
        m_rotation(Property_<glm::mat3>::create("rotation", glm::mat3()))
{
    register_property(m_object_index);
    register_property(m_enabled);
    register_property(m_position_x);
    register_property(m_position_y);
    register_property(m_position_z);
    register_property(m_scale);
    register_property(m_rotation);
    set_name("3D_Objects");
}

Object3DComponent::~Object3DComponent() {}

void Object3DComponent::update_property(const PropertyConstPtr &theProperty)
{
    gl::Object3DPtr active_object = m_objects.empty() ? gl::Object3DPtr() : m_objects[*m_object_index];
    if(!active_object)
    {
        LOG_ERROR << "could not update: no component set ...";
        return;
    }
    if(theProperty == m_object_index)
    {
        refresh();
    }else if(theProperty == m_enabled)
    {
        active_object->set_enabled(*m_enabled);
    }else if(theProperty == m_position_x || theProperty == m_position_y || theProperty == m_position_z)
    {
        active_object->set_position(glm::vec3(m_position_x->value(),
                                              m_position_y->value(),
                                              m_position_z->value()));
    }else if(theProperty == m_scale)
    {
        active_object->set_scale(*m_scale);
    }else if(theProperty == m_rotation)
    {
        active_object->set_rotation(*m_rotation);
    }
}

void Object3DComponent::set_objects(const std::vector<gl::Object3DPtr> &the_objects, bool copy_settings)
{
    m_objects.assign(the_objects.begin(), the_objects.end());
    m_object_index->set_range(0, the_objects.size() - 1);

    if(copy_settings)
    {
        observe_properties();
        m_object_index->set(*m_object_index);
    }
}

void Object3DComponent::set_objects(const std::vector<gl::MeshPtr> &the_objects,
                                    bool copy_settings)
{
    m_objects.clear();
    for(auto m : the_objects)
    {
        m_objects.push_back(std::dynamic_pointer_cast<gl::Object3D>(m));
    }

    m_object_index->set_range(0, m_objects.size() - 1);

    if(copy_settings)
    {
        observe_properties();
        m_object_index->set(*m_object_index);
    }
}

void Object3DComponent::set_index(int index)
{
    *m_object_index = index;
}

void Object3DComponent::refresh()
{
    observe_properties(false);

    gl::Object3DPtr active_object = m_objects.empty() ? gl::Object3DPtr() : m_objects[*m_object_index];
    if(!active_object)
    {
        LOG_ERROR << "could not refresh: no component set ...";
        return;
    }

    *m_enabled = active_object->enabled();
    *m_position_x = active_object->position().x;
    *m_position_y = active_object->position().y;
    *m_position_z = active_object->position().z;
    *m_scale = active_object->scale();
    *m_rotation = glm::mat3_cast(active_object->rotation());

    observe_properties(true);
}
}
