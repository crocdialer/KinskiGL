//
//  LightComponent.cpp
//  kinskiGL
//
//  Created by Fabian on 6/28/13.
//
//

#include "LightComponent.h"

namespace kinski
{
    LightComponent::LightComponent():
    m_light_index(RangedProperty<int>::create("index", -1, -1, 256)),
    m_light_type(RangedProperty<int>::create("light type", 0, 0, 2)),
    m_enabled(Property_<bool>::create("enabled", true)),
    m_position_x(Property_<float>::create("position X", 0)),
    m_position_y(Property_<float>::create("position Y", 0)),
    m_position_z(Property_<float>::create("position Z", 0)),
    m_direction(Property_<glm::vec3>::create("direction", glm::vec3(1))),
    m_ambient(Property_<gl::Color>::create("ambient", gl::Color(0))),
    m_diffuse(Property_<gl::Color>::create("diffuse", gl::Color())),
    m_specular(Property_<gl::Color>::create("specular", gl::Color())),
    m_att_constant(RangedProperty<float>::create("attenuation, constant", 1, 0, 1)),
    m_att_linear(RangedProperty<float>::create("attenuation, linear", 0, 0, 1.f)),
    m_att_quadratic(RangedProperty<float>::create("attenuation, quadratic", 0, 0, 1.f)),
    m_spot_cutoff(RangedProperty<float>::create("spot cutoff", 45.f, 0.f, 360.f)),
    m_spot_exponent(RangedProperty<float>::create("spot exponent", 0, 0, 256.f))
    {
        registerProperty(m_light_index);
        registerProperty(m_light_type);
        registerProperty(m_enabled);
        registerProperty(m_position_x);
        registerProperty(m_position_y);
        registerProperty(m_position_z);
        registerProperty(m_direction);
        registerProperty(m_ambient);
        registerProperty(m_diffuse);
        registerProperty(m_specular);
        registerProperty(m_att_constant);
        registerProperty(m_att_linear);
        registerProperty(m_att_quadratic);
        registerProperty(m_spot_cutoff);
        registerProperty(m_spot_exponent);
        set_name("Lights");
    }
    
    LightComponent::~LightComponent(){}
    
    void LightComponent::updateProperty(const Property::ConstPtr &theProperty)
    {
        gl::LightPtr active_light = m_lights.empty() ? gl::LightPtr() : m_lights[*m_light_index];
        if(!active_light) return;
        
        if(theProperty == m_light_index)
        {
            refresh();
        }
        else if(theProperty == m_light_type)
        {
            active_light->set_type(gl::Light::Type(m_light_type->value()));
        }
        else if(theProperty == m_enabled)
        {
            active_light->set_enabled(*m_enabled);
        }
        else if(theProperty == m_position_x || theProperty == m_position_y ||
                theProperty == m_position_z)
        {
            if(active_light->type() == gl::Light::DIRECTIONAL)
            {
                *m_direction = glm::normalize(-glm::vec3(*m_position_x, *m_position_y, *m_position_z));
            }
            else
            {
                active_light->setPosition(glm::vec3(*m_position_x, *m_position_y, *m_position_z));
            }
        }
        else if(theProperty == m_direction)
        {
            if(active_light->type() == gl::Light::DIRECTIONAL)
            {
                active_light->setPosition(m_direction->value());
            }
            active_light->setLookAt(active_light->position() + m_direction->value());
        }
        else if(theProperty == m_diffuse)
        {
            active_light->set_diffuse(*m_diffuse);
        }
        else if(theProperty == m_ambient)
        {
            active_light->set_ambient(*m_ambient);
        }
        else if(theProperty == m_specular)
        {
            active_light->set_specular(*m_specular);
        }
        else if(theProperty == m_att_constant || theProperty == m_att_linear ||
                theProperty == m_att_quadratic)
        {
            active_light->set_attenuation(*m_att_constant, *m_att_linear, *m_att_quadratic);
        }
        else if(theProperty == m_spot_cutoff)
        {
            active_light->set_spot_cutoff(*m_spot_cutoff);
        }
        else if(theProperty == m_spot_exponent)
        {
            active_light->set_spot_exponent(*m_spot_exponent);
        }
    }
    
    void LightComponent::set_index(int index)
    {
        *m_light_index = index;
    }
    
    void LightComponent::set_lights(const std::vector<gl::LightPtr> &l, bool copy_settings)
    {
        m_lights.assign(l.begin(), l.end());
        m_light_index->setRange(0, l.size() - 1);
        
        if(copy_settings)
        {
            observeProperties();
            m_light_index->set(*m_light_index);
        }
    }
    
    void LightComponent::refresh()
    {
        observeProperties(false);
        
        gl::LightPtr light = m_lights.empty() ? gl::LightPtr() : m_lights[*m_light_index];
        if(!light) return;
        
        *m_light_type = light->type();
        *m_enabled = light->enabled();
        *m_position_x = light->position().x;
        *m_position_y = light->position().y;
        *m_position_z = light->position().z;
        *m_direction = light->type() ? light->lookAt() : light->position();
        
        *m_diffuse = light->diffuse();
        *m_ambient = light->ambient();
        *m_specular = light->specular();
        *m_att_constant = light->attenuation().constant;
        *m_att_linear = light->attenuation().linear;
        *m_att_quadratic = light->attenuation().quadratic;
        *m_spot_cutoff = light->spot_cutoff();
        *m_spot_exponent = light->spot_exponent();
        
        observeProperties(true);
    }
}
