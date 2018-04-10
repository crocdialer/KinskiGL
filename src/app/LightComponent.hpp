// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  LightComponent.hpp
//
//  Created by Fabian Schmidt on 6/28/13.

#pragma once

#include "core/Component.hpp"
#include "gl/Light.hpp"

namespace kinski
{
    DEFINE_CLASS_PTR(LightComponent);
    
    class KINSKI_API LightComponent : public kinski::Component
    {
    public:

        static LightComponentPtr create();

        void update_property(const Property::ConstPtr &theProperty);
        void set_lights(const std::vector<gl::LightPtr> &l, bool copy_settings = true);
        void set_index(int index);
        void draw_light_dummies() const;
        void refresh();
        
    private:
        LightComponent();
        std::vector<gl::LightPtr> m_lights;
        std::vector<bool> m_draw_dummy_flags;
        Property_<bool>::Ptr m_draw_light_dummies;
        RangedProperty<int>::Ptr m_light_index;
        RangedProperty<int>::Ptr m_light_type;
        Property_<bool>::Ptr m_enabled;
        Property_<float>::Ptr m_intensity;
        Property_<float>::Ptr m_radius;
        Property_<bool>::Ptr m_cast_shadows;
        Property_<float>::Ptr m_position_x, m_position_y, m_position_z;
        Property_<glm::vec3>::Ptr m_direction;
        Property_<gl::Color>::Ptr m_ambient, m_diffuse;
        RangedProperty<float>::Ptr m_att_constant, m_att_linear, m_att_quadratic;
        RangedProperty<float>::Ptr m_spot_cutoff, m_spot_exponent;
    };
}
