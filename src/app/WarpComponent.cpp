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

#include "gl/Texture.hpp"
#include "app/App.hpp"
#include "WarpComponent.hpp"

namespace kinski
{
    WarpComponent::WarpComponent()
    {
        set_name("quad_warping");
        
        m_index = RangedProperty<uint32_t>::create("index", 0, 0, 9);
        m_enabled = Property_<bool>::create("enabled", false);
        m_num_subdivisions_x = RangedProperty<uint32_t>::create("num subdivisions x", 1, 1, 5);
        m_num_subdivisions_y = RangedProperty<uint32_t>::create("num subdivisions y", 1, 1, 5);
        m_draw_grid = Property_<bool>::create("draw grid", false);
        m_draw_control_points = Property_<bool>::create("draw control points", false);
        m_cubic_interpolation = Property_<bool>::create("use cubic interpolation", false);
        m_src_top_left = Property_<gl::vec2>::create("source area top left", gl::vec2(0));
        m_src_bottom_right = Property_<gl::vec2>::create("source area bottom right", gl::vec2(0));
        m_control_points = Property_<std::vector<gl::vec2>>::create("control points");
        m_control_points->set_tweakable(false);
        m_corners = Property_<std::vector<gl::vec2>>::create("quad corners");
        m_corners->set_tweakable(false);
        register_property(m_index);
        register_property(m_enabled);
        register_property(m_num_subdivisions_x);
        register_property(m_num_subdivisions_y);
        register_property(m_draw_grid);
        register_property(m_draw_control_points);
        register_property(m_cubic_interpolation);
        register_property(m_control_points);
        register_property(m_corners);
        register_property(m_src_top_left);
        register_property(m_src_bottom_right);
        
        register_function("reset", std::bind(&WarpComponent::reset, this));
        
        m_params.resize(m_quad_warp.size());
        m_params[0].enabled = true;
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
        *m_cubic_interpolation = the_quadwarp.cubic_interpolation();
        *m_num_subdivisions_x = the_quadwarp.num_subdivisions().x;
        *m_num_subdivisions_y = the_quadwarp.num_subdivisions().y;
        *m_control_points = the_quadwarp.control_points();
        *m_corners = the_quadwarp.corners();
        
        *m_src_top_left = gl::vec2(the_quadwarp.src_area().x0, the_quadwarp.src_area().y0);
        *m_src_bottom_right = gl::vec2(the_quadwarp.src_area().x1, the_quadwarp.src_area().y1);
        m_params[the_index].enabled = *m_enabled;
        m_params[the_index].display_grid = *m_draw_grid;
        m_params[the_index].display_points = *m_draw_control_points;
        m_quad_warp[the_index].set_num_subdivisions(the_quadwarp.num_subdivisions());
        m_quad_warp[the_index].set_control_points(*m_control_points);
        m_quad_warp[the_index].set_corners(*m_corners);
        m_quad_warp[the_index].set_cubic_interpolation(*m_cubic_interpolation);
        m_quad_warp[*m_index].set_src_area(Area_<uint32_t>(m_src_top_left->value().x,
                                                           m_src_top_left->value().y,
                                                           m_src_bottom_right->value().x,
                                                           m_src_bottom_right->value().y));
//        m_quad_warp[the_index].set_grid_resolution(the_quadwarp.grid_resolution());
//        m_active_control_points.clear();
//        
//        for(auto cp_index : quad_warp().selected_indices())
//        {
//            control_point_t cp(cp_index, quad_warp().control_points()[cp_index]);
//            m_active_control_points.insert(cp);
//        }
        
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
        else if(the_property == m_corners)
        {
            m_quad_warp[*m_index].set_corners(m_corners->value());
        }
        else if(the_property == m_num_subdivisions_x || the_property == m_num_subdivisions_y)
        {
            m_quad_warp[*m_index].set_num_subdivisions(*m_num_subdivisions_x, *m_num_subdivisions_y);
        }
        else if(the_property == m_src_bottom_right || the_property == m_src_top_left)
        {
            m_quad_warp[*m_index].set_src_area(Area_<uint32_t>(m_src_top_left->value().x,
                                                               m_src_top_left->value().y,
                                                               m_src_bottom_right->value().x,
                                                               m_src_bottom_right->value().y));
        }
        else if(the_property == m_cubic_interpolation)
        {
            m_quad_warp[*m_index].set_cubic_interpolation(*m_cubic_interpolation);
        }
    }
    
    void WarpComponent::render_output(int the_index, const gl::Texture &the_tex,
                                      const float the_brightness)
    {
        m_quad_warp[the_index].render_output(the_tex, the_brightness);
        if(m_params[the_index].display_grid){ m_quad_warp[the_index].render_grid(); }
        if(m_params[the_index].display_points){ m_quad_warp[the_index].render_control_points(); }
        
        if(m_show_cursor)
        {
            gl::vec2 cp = m_mouse_pos;
            gl::draw_line(gl::vec2(0, gl::window_dimension().y - cp.y),
                          gl::vec2(gl::window_dimension().x, gl::window_dimension().y - cp.y));
            gl::draw_line(gl::vec2(cp.x, 0), gl::vec2(cp.x, gl::window_dimension().y));
            
//            auto p = (m_quad_warp[the_index].transform() * gl::vec4(m_mouse_pos / gl::window_dimension(), 0, 1));
//            if(p.w != 0 ) { p /= p.w; }
//            p.xy() *= gl::window_dimension();
//            
//            gl::draw_circle(p.xy(), 5.f, gl::COLOR_ORANGE, true);
        }

    }
    
    void WarpComponent::key_press(const KeyEvent &e)
    {
        m_show_cursor = e.isAltDown();
        
        auto c = quad_warp().center();
        gl::vec2 inc = 1.f / gl::window_dimension();
        gl::ivec2 num_subs = quad_warp().num_subdivisions();
        
        switch(e.getCode())
        {
            case Key::_LEFT:
                for(auto cp : m_active_control_points)
                {
                    quad_warp().set_control_point(cp.index, quad_warp().control_point(cp.index) - gl::vec2(inc.x, 0.f));
                }
                if(m_active_control_points.empty())
                { quad_warp().move_center_to(gl::vec2(c.x - inc.x, c.y)); }
                break;
                
            case Key::_RIGHT:
                for(auto cp : m_active_control_points)
                {
                    quad_warp().set_control_point(cp.index, quad_warp().control_point(cp.index) + gl::vec2(inc.x, 0.f));
                }
                if(m_active_control_points.empty())
                { quad_warp().move_center_to(gl::vec2(c.x + inc.x, c.y)); }
                break;
                
            case Key::_UP:
                for(auto cp : m_active_control_points)
                {
                    quad_warp().set_control_point(cp.index, quad_warp().control_point(cp.index) + gl::vec2(0.f, inc.y));
                }
                if(m_active_control_points.empty())
                { quad_warp().move_center_to(gl::vec2(c.x, c.y + inc.y)); }
                break;
                
            case Key::_DOWN:
                for(auto cp : m_active_control_points)
                {
                    quad_warp().set_control_point(cp.index, quad_warp().control_point(cp.index) - gl::vec2(0.f, inc.y));
                }
                if(m_active_control_points.empty())
                { quad_warp().move_center_to(gl::vec2(c.x, c.y - inc.y)); }
                break;
                
            case Key::_F1:
                num_subs.x = std::max(num_subs.x - 1, 1);
                quad_warp().set_num_subdivisions(num_subs);
                m_active_control_points.clear();
                break;
                
            case Key::_F2:
                num_subs.x = std::max(num_subs.x + 1, 1);
                quad_warp().set_num_subdivisions(num_subs);
                m_active_control_points.clear();
                break;
                
            case Key::_F3:
                num_subs.y = std::max(num_subs.y - 1, 1);
                quad_warp().set_num_subdivisions(num_subs);
                m_active_control_points.clear();
                break;
                
            case Key::_F4:
                num_subs.y = std::max(num_subs.y + 1, 1);
                quad_warp().set_num_subdivisions(num_subs);
                m_active_control_points.clear();
                break;
                
            case Key::_1:
            case Key::_2:
            case Key::_3:
            case Key::_4:
            case Key::_5:
            case Key::_6:
            case Key::_7:
            case Key::_8:
            case Key::_9:
                if(e.isShiftDown())
                {
                    
                }
                set_index(e.getCode() - Key::_1);
                break;
                
            case Key::_F5:
                set_enabled(index(), !enabled(index()));
                break;
                
            case Key::_F6:
                set_display_grid(index(), !display_grid(index()));
                break;
                
            case Key::_F7:
                set_display_points(index(), !display_points(index()));
                break;
            
            case Key::_F8:
                *m_cubic_interpolation = !*m_cubic_interpolation;
                break;
                
            case Key::_F9:
                m_active_control_points.clear();
                reset();
                break;
        }
        refresh();
    }
    
    void WarpComponent::key_release(const KeyEvent &e)
    {
        m_show_cursor = e.isAltDown();
    }
    
    void WarpComponent::mouse_press(const MouseEvent &e)
    {
        m_click_pos = m_mouse_pos = glm::vec2(e.getX(), e.getY());
        
        if(e.isLeft() || e.is_touch())
        {
            auto coord = m_click_pos / gl::window_dimension();
            coord.y = 1.f - coord.y;
            auto px_length = 1.f / gl::window_dimension();

            const auto &control_points = quad_warp().control_points();

            for(uint32_t i = 0; i < control_points.size(); i ++)
            {
                auto c = quad_warp().control_point(i);
                
                if(glm::length(c - coord) < 15 * glm::length(px_length))
                {
                    control_point_t cp(i, c);
                    m_active_control_points.erase(cp);
                    m_active_control_points.insert(cp);

                    quad_warp().selected_indices().insert(i);
                    LOG_TRACE_2 << "selected control point: " << i << " -> " << glm::to_string(coord);
                }
            }
        }
        else if(e.isRight())
        {
            m_active_control_points.clear();
            quad_warp().selected_indices().clear();
        }
    }
    
    void WarpComponent::mouse_move(const MouseEvent &e)
    {
        m_mouse_pos = glm::vec2(e.getX(), e.getY());
    }
    
    void WarpComponent::mouse_drag(const MouseEvent &e)
    {
        m_mouse_pos = glm::vec2(e.getX(), e.getY());
        glm::vec2 mouseDiff = m_mouse_pos - m_click_pos;
        
        auto inc = mouseDiff / gl::window_dimension();
        inc.y *= -1.f;

        for(auto cp : m_active_control_points)
        {
            quad_warp().set_control_point(cp.index, cp.value + inc);
        }
    }
}
