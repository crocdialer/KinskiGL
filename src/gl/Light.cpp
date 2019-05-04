// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  Light.cpp
//
//  Created by Fabian Schmidt on 6/18/13.

//#include <cmath>
#include "Light.hpp"
#include "Visitor.hpp"

namespace kinski { namespace gl {
    
    LightPtr Light::create(Type theType)
    {
        return LightPtr(new Light(theType));
    }
    
    Light::Light(Type theType):
    Object3D(),
    m_type(theType),
    m_attenuation(Attenuation(1.f, 0.05f)),
    m_spot_cutoff(25.f),
    m_spot_exponent(1.f),
    m_intensity(1.f),
    m_radius(100.f),
    m_ambient(Color(0)),
    m_diffuse(Color(1)),
    m_cast_shadow(false)
    {
        set_name("Light_" + std::to_string(get_id()));
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
    
    void Light::set_intensity(float the_intensity)
    {
        m_intensity = crocore::clamp(the_intensity, 0.f, std::numeric_limits<float>::max());
    }
    
    void Light::set_radius(float the_radius)
    {
        m_radius = crocore::clamp(the_radius, 0.f, std::numeric_limits<float>::max());
    }
    
    const Light::Attenuation& Light::attenuation() const
    {
        return m_attenuation;
    }
    
    void Light::set_attenuation(const Attenuation &theAttenuation)
    {
        m_attenuation = Attenuation(std::max(theAttenuation.constant, 0.f),
                                    std::max(theAttenuation.quadratic, 0.f));
    }
    
    void Light::get_attenuation(float &constant, float &quadratic) const
    {
        constant = m_attenuation.constant;
        quadratic = m_attenuation.quadratic;
    }
    
    void Light::set_attenuation(float constant, float quadratic)
    {
        m_attenuation = Attenuation(std::max(constant, 0.f),
                                    std::max(quadratic, 0.f));
    }
    
    AABB Light::boundingBox() const
    {
        AABB ret;
        float d = radius();
        ret.min = glm::vec3(-d);
        ret.max = glm::vec3(d);
        ret.transform(global_transform());

        return ret;
    }
    
    void Light::accept(Visitor &theVisitor)
    {
        theVisitor.visit(*this);
    }
    
}}
