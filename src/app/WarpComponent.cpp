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
#include "gl/Texture.hpp"

namespace kinski
{
    WarpComponent::WarpComponent()
    {
        set_name("quad_warping");
        
        m_index = RangedProperty<uint32_t>::create("index", 0, 0, 9);
        m_enabled = Property_<bool>::create("enabled", false);
        m_grid_sz_x = RangedProperty<uint32_t>::create("grid size x", 16, 1, 512);
        m_grid_sz_y = RangedProperty<uint32_t>::create("grid size y", 9, 1, 512);
        m_draw_grid = Property_<bool>::create("draw grid", false);
        m_draw_control_points = Property_<bool>::create("draw control points", false);
        m_top_left = Property_<gl::vec2>::create("top left", gl::vec2(0.f, 0.f));
        m_top_right = Property_<gl::vec2>::create("top right", gl::vec2(1.f, 0.f));
        m_bottom_left = Property_<gl::vec2>::create("bottom left", gl::vec2(0.f, 1.f));
        m_bottom_right = Property_<gl::vec2>::create("bottom right", gl::vec2(1.f, 1.f));
        m_src_top_left = Property_<gl::vec2>::create("source area top left", gl::vec2(0));
        m_src_bottom_right = Property_<gl::vec2>::create("source area bottom right", gl::vec2(0));
        
        register_property(m_index);
        register_property(m_enabled);
        register_property(m_grid_sz_x);
        register_property(m_grid_sz_y);
        register_property(m_draw_grid);
        register_property(m_draw_control_points);
        register_property(m_top_left);
        register_property(m_top_right);
        register_property(m_bottom_left);
        register_property(m_bottom_right);
        register_property(m_src_top_left);
        register_property(m_src_bottom_right);
        
        register_function("reset", std::bind(&WarpComponent::reset, this));
        
        m_params.resize(m_quad_warp.size());
    }
    
    WarpComponent::~WarpComponent(){}
    
    void WarpComponent::reset()
    {
        *m_top_left = gl::vec2(0.f);
        *m_top_right = gl::vec2(1.f, 0.f);
        *m_bottom_left = gl::vec2(0.f, 1.f);
        *m_bottom_right = gl::vec2(1.f, 1.f);
        *m_src_top_left = gl::vec2(0);
        *m_src_bottom_right = gl::vec2(0);
    }
    
    void WarpComponent::set_enabled(int the_index, bool b)
    {
        *m_enabled = b;
    }
    
    bool WarpComponent::enabled(int the_index)
    {
        the_index = clamp<int>(the_index, 0, m_quad_warp.size() - 1);
        return m_params[the_index].enabled;
    }
    
    void WarpComponent::set_display_grid(int the_index, bool b)
    {
        *m_draw_grid = b;
    }
    
    void WarpComponent::set_display_points(int the_index, bool b)
    {
        *m_draw_control_points = b;
    }
    
    bool WarpComponent::display_grid(int the_index) const
    {
        the_index = clamp<int>(the_index, 0, m_quad_warp.size() - 1);
        return m_params[the_index].display_grid;
    }
    
    bool WarpComponent::display_points(int the_index) const
    {
        the_index = clamp<int>(the_index, 0, m_quad_warp.size() - 1);
        return m_params[the_index].display_points;
    }
    
    void WarpComponent::refresh()
    {
        set_from(m_quad_warp[*m_index], *m_index);
    }
    
    void WarpComponent::set_from(gl::QuadWarp &the_quadwarp, uint32_t the_index)
    {
        observe_properties(false);
        the_index = clamp<int>(the_index, 0, m_quad_warp.size() - 1);
        *m_index = the_index;
        *m_enabled = m_params[the_index].enabled;
        *m_draw_grid = m_params[the_index].display_grid;
        *m_draw_control_points = m_params[the_index].display_points;
        *m_top_left = the_quadwarp.control_point(0, 0);
        *m_top_right = the_quadwarp.control_point(1, 0);
        *m_bottom_left = the_quadwarp.control_point(0, 1);
        *m_bottom_right = the_quadwarp.control_point(1, 1);
        *m_grid_sz_x = the_quadwarp.grid_resolution().x;
        *m_grid_sz_y = the_quadwarp.grid_resolution().y;
        *m_src_top_left = gl::vec2(the_quadwarp.src_area().x0, the_quadwarp.src_area().y0);
        *m_src_bottom_right = gl::vec2(the_quadwarp.src_area().x1, the_quadwarp.src_area().y1);
        m_params[the_index].enabled = *m_enabled;
        m_params[the_index].display_grid = *m_draw_grid;
        m_params[the_index].display_points = *m_draw_control_points;
        m_quad_warp[the_index].control_point(0, 0) = *m_top_left;
        m_quad_warp[the_index].control_point(1, 0) = *m_top_right;
        m_quad_warp[the_index].control_point(0, 1) = *m_bottom_left;
        m_quad_warp[the_index].control_point(1, 1) = *m_bottom_right;
        m_quad_warp[the_index].set_grid_resolution(the_quadwarp.grid_resolution());
        m_quad_warp[*m_index].set_src_area(Area_<uint32_t>(m_src_top_left->value().x,
                                                           m_src_top_left->value().y,
                                                           m_src_bottom_right->value().x,
                                                           m_src_bottom_right->value().y));
        observe_properties(true);
    }
    
    gl::QuadWarp& WarpComponent::quad_warp(int i)
    {
        i = i < 0 || i >= (int)m_quad_warp.size() ? *m_index : i;
        return m_quad_warp[i];
    }
    
    void WarpComponent::update_property(const Property::ConstPtr &the_property)
    {
        if(the_property == m_index)
        {
            refresh();
        }
        else if(the_property == m_enabled)
        {
            m_params[*m_index].enabled = *m_enabled;
        }
        else if(the_property == m_draw_grid)
        {
            m_params[*m_index].display_grid = *m_draw_grid;
        }
        else if(the_property == m_draw_control_points)
        {
            m_params[*m_index].display_points = *m_draw_control_points;
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
        else if(the_property == m_grid_sz_x || the_property == m_grid_sz_y)
        {
            m_quad_warp[*m_index].set_grid_resolution(*m_grid_sz_x, *m_grid_sz_y);
        }
        else if(the_property == m_src_bottom_right || the_property == m_src_top_left)
        {
            m_quad_warp[*m_index].set_src_area(Area_<uint32_t>(m_src_top_left->value().x,
                                                               m_src_top_left->value().y,
                                                               m_src_bottom_right->value().x,
                                                               m_src_bottom_right->value().y));
        }
    }
    
    void WarpComponent::render_output(int the_index, const gl::Texture &the_tex, const float the_brightness)
    {
        m_quad_warp[the_index].render_output(the_tex, the_brightness);
        if(*m_draw_grid){ m_quad_warp[the_index].render_grid(); }
        if(*m_draw_control_points){ m_quad_warp[the_index].render_control_points(); }
    }
}
