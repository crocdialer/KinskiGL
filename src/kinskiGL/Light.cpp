//
//  Light.cpp
//  kinskiGL
//
//  Created by Fabian on 6/18/13.
//
//

#include "Light.h"

namespace kinski { namespace gl {

    Light::Light(Type theType):
    m_type(theType),
    m_attenuation(Attenuation(1.f, 0, 0)),
    m_ambient(Color(0)),
    m_diffuse(Color(1)),
    m_specular(Color(1)),
    m_enabled(true)
    {
    
    }
    
    Light::~Light()
    {
    
    }
    
    void Light::set_diffuse(const Color &theColor)
    {
        m_diffuse = glm::clamp(theColor, Color(0), Color(1));
    }
    
    void Light::set_ambient(const Color &theColor)
    {
        m_ambient = glm::clamp(theColor, Color(0), Color(1));
    }
    
    void Light::set_specular(const Color &theColor)
    {
        m_specular = glm::clamp(theColor, Color(0), Color(1));
    }
    
    const Light::Attenuation& Light::attenuation() const
    {
        return m_attenuation;
    }
    
    void Light::set_attenuation(const Attenuation &theAttenuation)
    {
        m_attenuation = Attenuation(std::max(theAttenuation.constant, 0.f),
                                    std::max(theAttenuation.linear, 0.f),
                                    std::max(theAttenuation.quadratic, 0.f));
    }
    
    void Light::get_attenuation(float &constant, float &linear, float &quadratic) const
    {
        constant = m_attenuation.constant;
        linear = m_attenuation.linear;
        quadratic = m_attenuation.quadratic;
    }
    
    void Light::set_attenuation(float constant, float linear, float quadratic)
    {
        m_attenuation = Attenuation(std::max(constant, 0.f),
                                    std::max(linear, 0.f),
                                    std::max(quadratic, 0.f));
    }
    
    void Light::accept(Visitor &theVisitor)
    {
        theVisitor.visit(*this);
    }
    
}}