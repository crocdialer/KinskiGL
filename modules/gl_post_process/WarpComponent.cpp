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
        
        m_index = RangedProperty<uint32_t>::create("index", 0, 0, 9);
        m_grid_sz_x = RangedProperty<uint32_t>::create("grid size x", 16, 1, 512);
        m_grid_sz_y = RangedProperty<uint32_t>::create("grid size y", 9, 1, 512);
        m_draw_grid = Property_<bool>::create("draw grid", false);
        m_draw_control_points = Property_<bool>::create("draw control points", false);
        m_top_left = Property_<gl::vec2>::create("top left", gl::vec2(0.f, 0.f));
        m_top_right = Property_<gl::vec2>::create("top right", gl::vec2(1.f, 0.f));
        m_bottom_left = Property_<gl::vec2>::create("bottom left", gl::vec2(0.f, 1.f));
        m_bottom_right = Property_<gl::vec2>::create("bottom right", gl::vec2(1.f, 1.f));
        
        register_property(m_index);
        register_property(m_grid_sz_x);
        register_property(m_grid_sz_y);
        register_property(m_draw_grid);
        register_property(m_draw_control_points);
        register_property(m_top_left);
        register_property(m_top_right);
        register_property(m_bottom_left);
        register_property(m_bottom_right);
        
        register_function("reset", std::bind(&WarpComponent::reset, this));
    }
    
    WarpComponent::~WarpComponent(){}
    
    void WarpComponent::reset()
    {
        *m_top_left = gl::vec2(0.f);
        *m_top_right = gl::vec2(1.f, 0.f);
        *m_bottom_left = gl::vec2(0.f, 1.f);
        *m_bottom_right = gl::vec2(1.f, 1.f);
    }
    
    void WarpComponent::refresh()
    {
        observe_properties(false);
        *m_top_left = m_quad_warp[*m_index].control_point(0, 0);
        *m_top_right = m_quad_warp[*m_index].control_point(1, 0);
        *m_bottom_left = m_quad_warp[*m_index].control_point(0, 1);
        *m_bottom_right = m_quad_warp[*m_index].control_point(1, 1);
        observe_properties(true);
    }
    
    void WarpComponent::update_property(const Property::ConstPtr &the_property)
    {
        if(the_property == m_index)
        {
            refresh();
        }
        else if(the_property == m_top_left)
        {
            m_quad_warp[*m_index].control_point(0, 0) = *m_top_left;
        }
        else if(the_property == m_top_right)
        {
            m_quad_warp[*m_index].control_point(1, 0) = *m_top_right;;
        }
        else if(the_property == m_bottom_left)
        {
            m_quad_warp[*m_index].control_point(0, 1) = *m_bottom_left;
        }
        else if(the_property == m_bottom_right)
        {
            m_quad_warp[*m_index].control_point(1, 1) = *m_bottom_right;
        }
        if(the_property == m_grid_sz_x || the_property == m_grid_sz_y)
        {
            m_quad_warp[*m_index].set_grid_resolution(*m_grid_sz_x, *m_grid_sz_y);
        }
    }
    
    void WarpComponent::render_output(const gl::Texture &the_tex, const float the_brightness)
    {
        m_quad_warp[*m_index].render_output(the_tex, the_brightness);
        if(*m_draw_grid){ m_quad_warp[*m_index].render_grid(); }
        if(*m_draw_control_points){ m_quad_warp[*m_index].render_control_points(); }
    }
}
