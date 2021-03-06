// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  LightComponent.cpp
//
//  Created by Fabian on 6/28/13.

#include "LightComponent.hpp"

using namespace crocore;

namespace kinski {

LightComponentPtr LightComponent::create()
{
    return LightComponentPtr(new LightComponent());
}

LightComponent::LightComponent() :
        m_draw_light_dummies(Property_<bool>::create("draw light dummies", false)),
        m_light_index(RangedProperty<int>::create("index", -1, -1, 256)),
        m_light_type(RangedProperty<int>::create("light type", 0, 0, 2)),
        m_enabled(Property_<bool>::create("enabled", true)),
        m_intensity(Property_<float>::create("intensity", 1.f)),
        m_radius(Property_<float>::create("radius", 1.f)),
        m_cast_shadows(Property_<bool>::create("cast shadows", false)),
        m_position_x(Property_<float>::create("position X", 0)),
        m_position_y(Property_<float>::create("position Y", 0)),
        m_position_z(Property_<float>::create("position Z", 0)),
        m_direction(Property_<glm::vec3>::create("direction", glm::vec3(1))),
        m_ambient(Property_<gl::Color>::create("ambient", gl::Color(0))),
        m_diffuse(Property_<gl::Color>::create("diffuse", gl::Color())),
        m_att_constant(RangedProperty<float>::create("attenuation, constant", 1, 0, 10)),
        m_att_quadratic(RangedProperty<float>::create("attenuation, quadratic", 0, 0, 10.f)),
        m_spot_cutoff(RangedProperty<float>::create("spot cutoff", 45.f, 0.f, 89.99f)),
        m_spot_exponent(RangedProperty<float>::create("spot exponent", 0, 0, 256.f))
{
    register_property(m_draw_light_dummies);
    register_property(m_light_index);
    register_property(m_light_type);
    register_property(m_enabled);
    register_property(m_intensity);
    register_property(m_radius);
    register_property(m_cast_shadows);
    register_property(m_position_x);
    register_property(m_position_y);
    register_property(m_position_z);
    register_property(m_direction);
    register_property(m_ambient);
    register_property(m_diffuse);
    register_property(m_att_constant);
    register_property(m_att_quadratic);
    register_property(m_spot_cutoff);
    register_property(m_spot_exponent);
    set_name("Lights");
}

void LightComponent::update_property(const PropertyConstPtr &theProperty)
{
    gl::LightPtr active_light = m_lights.empty() ? gl::LightPtr() : m_lights[*m_light_index];
    if(!active_light) return;

    if(theProperty == m_light_index)
    {
        refresh();
    }else if(theProperty == m_light_type)
    {
        active_light->set_type(gl::Light::Type(m_light_type->value()));
    }else if(theProperty == m_enabled)
    {
        active_light->set_enabled(*m_enabled);
    }else if(theProperty == m_intensity)
    {
        active_light->set_intensity(*m_intensity);
    }else if(theProperty == m_radius)
    {
        active_light->set_radius(*m_radius);
    }else if(theProperty == m_cast_shadows)
    {
        active_light->set_cast_shadow(*m_cast_shadows);
    }else if(theProperty == m_position_x || theProperty == m_position_y ||
             theProperty == m_position_z)
    {
        active_light->set_position(glm::vec3(m_position_x->value(),
                                             m_position_y->value(),
                                             m_position_z->value()));

    }else if(theProperty == m_direction)
    {
        auto p = active_light->position() + m_direction->value();
        active_light->set_look_at(p);
    }else if(theProperty == m_diffuse)
    {
        active_light->set_diffuse(*m_diffuse);
    }else if(theProperty == m_ambient)
    {
        active_light->set_ambient(*m_ambient);
    }else if(theProperty == m_att_constant ||
             theProperty == m_att_quadratic)
    {
        active_light->set_attenuation(*m_att_constant, *m_att_quadratic);
    }else if(theProperty == m_spot_cutoff)
    {
        active_light->set_spot_cutoff(*m_spot_cutoff);
    }else if(theProperty == m_spot_exponent)
    {
        active_light->set_spot_exponent(*m_spot_exponent);
    }else if(theProperty == m_draw_light_dummies)
    {
        m_draw_dummy_flags[*m_light_index] = *m_draw_light_dummies;
    }
}

void LightComponent::set_index(int index)
{
    *m_light_index = index;
}

gl::LightPtr LightComponent::current_light() const
{
    return m_lights[*m_light_index];
}

void LightComponent::draw_light_dummies() const
{
    for(uint32_t i = 0; i < m_lights.size(); ++i)
    {
        if(m_draw_dummy_flags[i]){ gl::draw_light(m_lights[i]); }
    }
}

void LightComponent::set_lights(const std::vector<gl::LightPtr> &l, bool copy_settings)
{
    m_lights.assign(l.begin(), l.end());
    m_light_index->set_range(0, l.size() - 1);
    m_draw_dummy_flags.resize(m_lights.size(), false);

    if(copy_settings)
    {
        observe_properties();
        m_light_index->set(*m_light_index);
    }
}

bool LightComponent::use_dummy(size_t the_index) const
{
    if(the_index < m_draw_dummy_flags.size())
    {
        return m_draw_dummy_flags[the_index];
    }
    return false;
}

void LightComponent::set_use_dummy(size_t the_index, bool b)
{
    if(the_index < m_draw_dummy_flags.size()){ m_draw_dummy_flags[the_index] = b; }
}

void LightComponent::refresh()
{
    observe_properties(false);

    gl::LightPtr light = m_lights.empty() ? gl::LightPtr() : m_lights[*m_light_index];
    if(!light) return;

    *m_draw_light_dummies = m_draw_dummy_flags[*m_light_index];

    *m_light_type = light->type();
    *m_enabled = light->enabled();
    *m_intensity = light->intensity();
    *m_radius = light->radius();
    *m_cast_shadows = light->cast_shadow();
    *m_position_x = light->position().x;
    *m_position_y = light->position().y;
    *m_position_z = light->position().z;
    *m_direction = light->lookAt();

    *m_diffuse = light->diffuse();
    *m_ambient = light->ambient();
    *m_att_constant = light->attenuation().constant;
    *m_att_quadratic = light->attenuation().quadratic;
    *m_spot_cutoff = light->spot_cutoff();
    *m_spot_exponent = light->spot_exponent();
    observe_properties(true);
}
}// namespace kinski
