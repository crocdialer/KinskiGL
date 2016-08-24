// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  WarpComponent
//
//  Created by Fabian Schmidt on 12/05/15.

#include "WarpComponent.hpp"

namespace kinski
{
    WarpComponent::WarpComponent()
    {
        set_name("quad_warping");
        
        m_grid_sz_x = RangedProperty<uint32_t>::create("grid size x", 16, 1, 512);
        m_grid_sz_y = RangedProperty<uint32_t>::create("grid size y", 9, 1, 512);
        m_draw_grid = Property_<bool>::create("draw grid", false);
        m_draw_control_points = Property_<bool>::create("draw control points", false);
        m_top_left_x = RangedProperty<float>::create("top left x", 0.f, -.2f, 1.2f);
        m_top_right_x = RangedProperty<float>::create("top right x", 1.f, -.2f, 1.2f);
        m_bottom_left_x = RangedProperty<float>::create("bottom left x", 0.f, -.2f, 1.2f);
        m_bottom_right_x = RangedProperty<float>::create("bottom right x", 1.f, -.2f, 1.2f);

        m_top_left_y = RangedProperty<float>::create("top left y", 0.f, -.2f, 1.2f);
        m_top_right_y = RangedProperty<float>::create("top right y", 0.f, -.2f, 1.2f);
        m_bottom_left_y = RangedProperty<float>::create("bottom left y", 1.f, -.2f, 1.2f);
        m_bottom_right_y = RangedProperty<float>::create("bottom right y", 1.f, -.2f, 1.2f);
        
        register_property(m_grid_sz_x);
        register_property(m_grid_sz_y);
        register_property(m_draw_grid);
        register_property(m_draw_control_points);
        register_property(m_top_left_x);
        register_property(m_top_left_y);
        register_property(m_top_right_x);
        register_property(m_top_right_y);
        register_property(m_bottom_left_x);
        register_property(m_bottom_left_y);
        register_property(m_bottom_right_x);
        register_property(m_bottom_right_y);
        
        register_function("reset", std::bind(&WarpComponent::reset, this));
    }
    
    WarpComponent::~WarpComponent(){}
    
    void WarpComponent::reset()
    {
        *m_top_left_x = 0.f;
        *m_top_right_x = 1.f;
        *m_bottom_left_x = 0.f;
        *m_bottom_right_x = 1.f;
        
        *m_top_left_y = 0.f;
        *m_top_right_y = 0.f;
        *m_bottom_left_y = 1.f;
        *m_bottom_right_y = 1.f;
    }
    
    void WarpComponent::update_property(const Property::ConstPtr &the_property)
    {
        if(the_property == m_top_left_x || the_property == m_top_left_y)
        {
            m_quad_warp.control_point(0, 0) = gl::vec2(*m_top_left_x, *m_top_left_y);
        }
        else if(the_property == m_top_right_x || the_property == m_top_right_y)
        {
            m_quad_warp.control_point(1, 0) = gl::vec2(*m_top_right_x, *m_top_right_y);
        }
        else if(the_property == m_bottom_left_x || the_property == m_bottom_left_y)
        {
            m_quad_warp.control_point(0, 1) = gl::vec2(*m_bottom_left_x, *m_bottom_left_y);
        }
        else if(the_property == m_bottom_right_x || the_property == m_bottom_right_y)
        {
            m_quad_warp.control_point(1, 1) = gl::vec2(*m_bottom_right_x, *m_bottom_right_y);
        }
        if(the_property == m_grid_sz_x || the_property == m_grid_sz_y)
        {
            m_quad_warp.set_grid_resolution(*m_grid_sz_x, *m_grid_sz_y);
        }
    }
    
    void WarpComponent::render_output(const gl::Texture &the_tex, const float the_brightness)
    {
        m_quad_warp.render_output(the_tex);
        if(*m_draw_grid){ m_quad_warp.render_grid(); }
        if(*m_draw_control_points){ m_quad_warp.render_control_points(); }
    }
}
