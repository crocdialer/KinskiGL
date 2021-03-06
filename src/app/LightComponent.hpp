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

#include <crocore/Component.hpp>
#include "gl/Light.hpp"

namespace kinski
{
    DEFINE_CLASS_PTR(LightComponent);
    
    class LightComponent : public crocore::Component
    {
    public:

        static LightComponentPtr create();

        void update_property(const crocore::PropertyConstPtr &theProperty);
        void set_lights(const std::vector<gl::LightPtr> &l, bool copy_settings = true);
        const std::vector<gl::LightPtr>& lights() const { return m_lights; };
        void set_index(int index);
        gl::LightPtr current_light() const;
        void draw_light_dummies() const;
        bool use_dummy(size_t the_index) const;
        void set_use_dummy(size_t the_index, bool b);
        void refresh();
        
    private:
        LightComponent();
        std::vector<gl::LightPtr> m_lights;
        std::vector<bool> m_draw_dummy_flags;
        crocore::Property_<bool>::Ptr m_draw_light_dummies;
        crocore::RangedProperty<int>::Ptr m_light_index;
        crocore::RangedProperty<int>::Ptr m_light_type;
        crocore::Property_<bool>::Ptr m_enabled;
        crocore::Property_<float>::Ptr m_intensity;
        crocore::Property_<float>::Ptr m_radius;
        crocore::Property_<bool>::Ptr m_cast_shadows;
        crocore::Property_<float>::Ptr m_position_x, m_position_y, m_position_z;
        crocore::Property_<glm::vec3>::Ptr m_direction;
        crocore::Property_<gl::Color>::Ptr m_ambient, m_diffuse;
        crocore::RangedProperty<float>::Ptr m_att_constant, m_att_quadratic;
        crocore::RangedProperty<float>::Ptr m_spot_cutoff, m_spot_exponent;
    };
}
