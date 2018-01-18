// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  Light.hpp
//
//  Created by Fabian on 6/18/13.

#pragma once

#include "Object3D.hpp"

namespace kinski { namespace gl {
    
    class KINSKI_API Light : public Object3D
    {
    public:
        
        //! type enum
        enum Type {DIRECTIONAL = 0, POINT = 1, SPOT = 2, AREA = 3, UNKNOWN = 4};
        
        //! Helper struct to bundle attenuation params
        struct Attenuation
        {
            float constant, linear, quadratic;
            Attenuation(float c, float l, float q):constant(c), linear(l), quadratic(q){}
        };
        
        static LightPtr create(Type theType);
        
        virtual ~Light();
        
        const Color& diffuse() const { return m_diffuse; };
        const Color& ambient() const { return m_ambient; };
        const Color& specular() const { return m_specular; };
        void set_diffuse(const Color &theColor);
        void set_ambient(const Color &theColor);
        void set_specular(const Color &theColor);
        float intensity() const { return m_intensity; }
        void set_intensity(float the_intensity);
        
        float radius() const { return m_radius; }
        void set_radius(float the_radius);
        
        const Attenuation& attenuation() const;
        void set_attenuation(const Attenuation &theAttenuation);
        void get_attenuation(float &constant, float &linear, float &quadratic) const;
        void set_attenuation(float constant, float linear, float quadratic);
        float spot_exponent() const {return m_spot_exponent;}
        float spot_cutoff() const {return m_spot_cutoff;}
        void set_spot_exponent(float f){m_spot_exponent = f;}
        void set_spot_cutoff(float f){m_spot_cutoff = f;}
        Type type() const {return m_type;}
        void set_type(Type theType){m_type = theType;}
        void set_cast_shadow(bool b){ m_cast_shadow = b; }
        bool cast_shadow() const { return m_cast_shadow; }
        
        //! corresponds to the light´s area of effect
        gl::AABB boundingBox() const;
        
        void accept(Visitor &theVisitor);
        
    private:
        
        Light(Type theType);
        
        Type m_type;
        Attenuation m_attenuation;
        float m_spot_cutoff, m_spot_exponent, m_intensity, m_radius;
        Color m_ambient, m_diffuse, m_specular;
        bool m_cast_shadow;
    };
    
}}//namespace
