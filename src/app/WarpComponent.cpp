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
        m_grid_sz_x = RangedProperty<uint32_t>::create("grid size x", 1, 1, 5);
        m_grid_sz_y = RangedProperty<uint32_t>::create("grid size y", 1, 1, 5);
        m_draw_grid = Property_<bool>::create("draw grid", false);
        m_draw_control_points = Property_<bool>::create("draw control points", false);
        m_src_top_left = Property_<gl::vec2>::create("source area top left", gl::vec2(0));
        m_src_bottom_right = Property_<gl::vec2>::create("source area bottom right", gl::vec2(0));
        m_control_points = Property_<std::vector<gl::vec2>>::create("control points");
        m_control_points->set_tweakable(false);
        register_property(m_index);
        register_property(m_enabled);
        register_property(m_grid_sz_x);
        register_property(m_grid_sz_y);
        register_property(m_draw_grid);
        register_property(m_draw_control_points);
        register_property(m_control_points);
        register_property(m_src_top_left);
        register_property(m_src_bottom_right);
        
        register_function("reset", std::bind(&WarpComponent::reset, this));
        
        m_params.resize(m_quad_warp.size());
    }
    
    WarpComponent::~WarpComponent(){}
    
    void WarpComponent::reset()
    {
        *m_src_top_left = gl::vec2(0);
        *m_src_bottom_right = gl::vec2(0);
        m_quad_warp[*m_index].reset();
        refresh();
        
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
        *m_grid_sz_x = the_quadwarp.num_subdivisions().x;
        *m_grid_sz_y = the_quadwarp.num_subdivisions().y;
        *m_control_points = the_quadwarp.control_points();
        *m_src_top_left = gl::vec2(the_quadwarp.src_area().x0, the_quadwarp.src_area().y0);
        *m_src_bottom_right = gl::vec2(the_quadwarp.src_area().x1, the_quadwarp.src_area().y1);
        m_params[the_index].enabled = *m_enabled;
        m_params[the_index].display_grid = *m_draw_grid;
        m_params[the_index].display_points = *m_draw_control_points;
        m_quad_warp[the_index].set_num_subdivisions(the_quadwarp.num_subdivisions());
        m_quad_warp[the_index].set_control_points(*m_control_points);
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
    
    uint32_t WarpComponent::num_warps() const
    {
        return m_quad_warp.size();
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
        else if(the_property == m_control_points)
        {
            m_quad_warp[*m_index].set_control_points(m_control_points->value());
        }
        else if(the_property == m_grid_sz_x || the_property == m_grid_sz_y)
        {
            m_quad_warp[*m_index].set_num_subdivisions(*m_grid_sz_x, *m_grid_sz_y);
        }
        else if(the_property == m_src_bottom_right || the_property == m_src_top_left)
        {
            m_quad_warp[*m_index].set_src_area(Area_<uint32_t>(m_src_top_left->value().x,
                                                               m_src_top_left->value().y,
                                                               m_src_bottom_right->value().x,
                                                               m_src_bottom_right->value().y));
        }
    }
    
    void WarpComponent::render_output(int the_index, const gl::Texture &the_tex,
                                      const float the_brightness)
    {
        m_quad_warp[the_index].render_output(the_tex, the_brightness);
        if(m_params[the_index].display_grid){ m_quad_warp[the_index].render_grid(); }
        if(m_params[the_index].display_points){ m_quad_warp[the_index].render_control_points(); }
    }
}
