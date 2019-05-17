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

using namespace crocore;

namespace kinski {

WarpComponentPtr WarpComponent::create()
{
    return WarpComponentPtr(new WarpComponent());
}

WarpComponent::WarpComponent()
{
    set_name("quad_warping");

    m_index = RangedProperty<int>::create("index", 0, 0, 9);
    m_enabled = Property_<bool>::create("enabled", false);
    m_num_subdivisions = Property_<glm::ivec2>::create("num subdivisions", glm::ivec2(1));
    m_grid_resolution = Property_<gl::ivec2>::create("grid resolution", gl::ivec2(32, 18));
    m_draw_grid = Property_<bool>::create("draw grid", false);
    m_draw_control_points = Property_<bool>::create("draw control points", false);
    m_perspective = Property_<bool>::create("perspective", true);
    m_cubic_interpolation = Property_<bool>::create("use cubic interpolation", false);
    m_src_top_left = Property_<gl::vec2>::create("source area top left", gl::vec2(0));
    m_src_size = Property_<gl::vec2>::create("source area size", gl::vec2(1));
    m_control_points = Property_<std::vector<gl::vec2>>::create("control points");
    m_control_points->set_tweakable(false);
    m_corners = Property_<std::vector<gl::vec2>>::create("quad corners");
    m_corners->set_tweakable(false);
    m_edges = Property_<std::vector<float>>::create("edges", std::vector<float>(4, 0.f));
    m_edges->set_tweakable(false);
    m_edge_exponents = Property_<std::vector<float>>::create("edge exponents", std::vector<float>(4, 1.f));
    m_edge_exponents->set_tweakable(false);

    register_property(m_index);
    register_property(m_enabled);
    register_property(m_num_subdivisions);
    register_property(m_draw_grid);
    register_property(m_draw_control_points);
    register_property(m_perspective);
    register_property(m_cubic_interpolation);
    register_property(m_control_points);
    register_property(m_corners);
    register_property(m_src_top_left);
    register_property(m_src_size);
    register_property(m_grid_resolution);
    register_property(m_edges);
    register_property(m_edge_exponents);

    register_function("reset", [this](const std::vector<std::string> &) { reset(); });

    m_params.resize(m_quad_warp.size());
    m_params[0].enabled = true;
}

void WarpComponent::reset()
{
    *m_src_top_left = gl::vec2(0);
    *m_src_size = gl::vec2(0);
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

void WarpComponent::set_from(gl::Warp &the_quadwarp, uint32_t the_index)
{
    observe_properties(false);
    the_index = clamp<int>(the_index, 0, m_quad_warp.size() - 1);
    *m_index = the_index;
    *m_enabled = m_params[the_index].enabled;
    *m_draw_grid = m_params[the_index].display_grid;
    *m_draw_control_points = m_params[the_index].display_points;
    *m_cubic_interpolation = the_quadwarp.cubic_interpolation();
    *m_perspective = the_quadwarp.perspective();
    *m_num_subdivisions = the_quadwarp.num_subdivisions();
    *m_control_points = the_quadwarp.control_points();
    *m_corners = the_quadwarp.corners();

    *m_src_top_left = gl::vec2(the_quadwarp.src_area().x, the_quadwarp.src_area().y);
    *m_src_size = gl::vec2(the_quadwarp.src_area().width, the_quadwarp.src_area().height);

    *m_grid_resolution = the_quadwarp.grid_resolution();

    auto edges = the_quadwarp.edges(), edge_exp = the_quadwarp.edge_exponents();
    *m_edges = {edges.x, edges.y, edges.z, edges.w};
    *m_edge_exponents = {edge_exp.x, edge_exp.y, edge_exp.z, edge_exp.w};

    m_params[the_index].enabled = *m_enabled;
    m_params[the_index].display_grid = *m_draw_grid;
    m_params[the_index].display_points = *m_draw_control_points;
    m_quad_warp[the_index].set_num_subdivisions(the_quadwarp.num_subdivisions());
    m_quad_warp[the_index].set_control_points(*m_control_points);
    m_quad_warp[the_index].set_corners(*m_corners);
    m_quad_warp[the_index].set_perspective(*m_perspective);
    m_quad_warp[the_index].set_cubic_interpolation(*m_cubic_interpolation);
    m_quad_warp[the_index].set_src_area({m_src_top_left->value().x,
                                         m_src_top_left->value().y,
                                         m_src_size->value().x,
                                         m_src_size->value().y});
    m_quad_warp[the_index].set_grid_resolution(the_quadwarp.grid_resolution());

    if(m_edges->value().size() == 4)
    {
        const auto &vec = m_edges->value();
        gl::vec4 tmp(vec[0], vec[1], vec[2], vec[3]);
        m_quad_warp[the_index].set_edges(tmp);
    }
    if(m_edge_exponents->value().size() == 4)
    {
        const auto &vec = m_edge_exponents->value();
        gl::vec4 tmp(vec[0], vec[1], vec[2], vec[3]);
        m_quad_warp[the_index].set_edge_exponents(tmp);
    }

    observe_properties(true);
}

gl::Warp &WarpComponent::quad_warp(int i)
{
    i = i < 0 || i >= (int)m_quad_warp.size() ? *m_index : i;
    return m_quad_warp[i];
}

uint32_t WarpComponent::num_warps() const
{
    return m_quad_warp.size();
}

void WarpComponent::update_property(const PropertyConstPtr &the_property)
{
    if(the_property == m_index)
    {
        refresh();
    }else if(the_property == m_enabled)
    {
        m_params[*m_index].enabled = *m_enabled;
    }else if(the_property == m_draw_grid)
    {
        m_params[*m_index].display_grid = *m_draw_grid;
    }else if(the_property == m_draw_control_points)
    {
        m_params[*m_index].display_points = *m_draw_control_points;
    }else if(the_property == m_control_points)
    {
        m_quad_warp[*m_index].set_control_points(m_control_points->value());
    }else if(the_property == m_corners)
    {
        m_quad_warp[*m_index].set_corners(m_corners->value());
    }else if(the_property == m_num_subdivisions)
    {
        m_quad_warp[*m_index].set_num_subdivisions(*m_num_subdivisions);
    }else if(the_property == m_grid_resolution)
    {
        m_quad_warp[*m_index].set_grid_resolution(*m_grid_resolution);
    }else if(the_property == m_src_size || the_property == m_src_top_left)
    {
        m_quad_warp[*m_index].set_src_area({m_src_top_left->value().x,
                                            m_src_top_left->value().y,
                                            m_src_size->value().x,
                                            m_src_size->value().y});
    }else if(the_property == m_perspective)
    {
        m_quad_warp[*m_index].set_perspective(*m_perspective);
    }else if(the_property == m_cubic_interpolation)
    {
        m_quad_warp[*m_index].set_cubic_interpolation(*m_cubic_interpolation);
    }else if(the_property == m_edges)
    {
        if(m_edges->value().size() == 4)
        {
            const auto &vec = m_edges->value();
            gl::vec4 tmp(vec[0], vec[1], vec[2], vec[3]);
            m_quad_warp[*m_index].set_edges(tmp);
        }
    }else if(the_property == m_edge_exponents)
    {
        if(m_edge_exponents->value().size() == 4)
        {
            const auto &vec = m_edge_exponents->value();
            gl::vec4 tmp(vec[0], vec[1], vec[2], vec[3]);
            m_quad_warp[*m_index].set_edge_exponents(tmp);
        }
    }
}

void WarpComponent::render_output(int the_index, const gl::Texture &the_tex,
                                  const float the_brightness)
{
    m_quad_warp[the_index].render_output(the_tex, the_brightness);
    if(m_params[the_index].display_grid){ m_quad_warp[the_index].render_grid(); }

    if(m_show_cursor)
    {
        auto col = (the_index == *m_index) ? gl::COLOR_RED : gl::COLOR_WHITE;

        // boundary
        m_quad_warp[the_index].render_boundary(col);

        // label
        auto p = m_quad_warp[the_index].transform() * gl::vec4(0.025f, 1.f, 0, 1);
        p /= p.w;
        p.y = 1.f - p.y;
        gl::draw_text_2D("warp_" + to_string(the_index + 1), m_font, col,
                         p.xy() * gl::window_dimension());

        if(the_index == *m_index)
        {
            // control points
            m_quad_warp[the_index].render_control_points();
        }

        // cursor
        gl::vec2 cp = m_mouse_pos;
        gl::draw_line(gl::vec2(0, gl::window_dimension().y - cp.y),
                      gl::vec2(gl::window_dimension().x, gl::window_dimension().y - cp.y));
        gl::draw_line(gl::vec2(cp.x, 0), gl::vec2(cp.x, gl::window_dimension().y));
    }else if(m_params[the_index].display_points && (the_index == *m_index))
    {
        m_quad_warp[the_index].render_control_points();
    }
}

void WarpComponent::key_press(const KeyEvent &e)
{
    m_show_cursor = e.is_alt_down();

    auto c = quad_warp().center();
    gl::vec2 inc = (e.is_shift_down() ? 10.f : 1.f) / gl::window_dimension();
    gl::ivec2 num_subs = quad_warp().num_subdivisions();

    switch(e.code())
    {
        case Key::_LEFT:
            for(auto cp : m_active_control_points)
            {
                quad_warp().set_control_point(cp.index, quad_warp().control_point(cp.index) - gl::vec2(inc.x, 0.f));
            }
            if(m_active_control_points.empty()){ quad_warp().move_center_to(gl::vec2(c.x - inc.x, c.y)); }
            break;

        case Key::_RIGHT:
            for(auto cp : m_active_control_points)
            {
                quad_warp().set_control_point(cp.index, quad_warp().control_point(cp.index) + gl::vec2(inc.x, 0.f));
            }
            if(m_active_control_points.empty()){ quad_warp().move_center_to(gl::vec2(c.x + inc.x, c.y)); }
            break;

        case Key::_UP:
            for(auto cp : m_active_control_points)
            {
                quad_warp().set_control_point(cp.index, quad_warp().control_point(cp.index) + gl::vec2(0.f, inc.y));
            }
            if(m_active_control_points.empty()){ quad_warp().move_center_to(gl::vec2(c.x, c.y + inc.y)); }
            break;

        case Key::_DOWN:
            for(auto cp : m_active_control_points)
            {
                quad_warp().set_control_point(cp.index, quad_warp().control_point(cp.index) - gl::vec2(0.f, inc.y));
            }
            if(m_active_control_points.empty()){ quad_warp().move_center_to(gl::vec2(c.x, c.y - inc.y)); }
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
            if(e.is_alt_down())
            {
                set_index(e.code() - Key::_1);
            }
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

        case Key::_F10:
            quad_warp().rotate_content();
            break;

        case Key::_F11:
            quad_warp().flip_content(true);
            break;

        case Key::_F12:
            quad_warp().flip_content(false);
            break;
    }
    refresh();
}

void WarpComponent::key_release(const KeyEvent &e)
{
    m_show_cursor = e.is_alt_down();
}

void WarpComponent::mouse_press(const MouseEvent &e)
{
    m_click_pos = m_mouse_pos = glm::vec2(e.get_x(), e.get_y());

    if(e.is_left() || e.is_touch())
    {
        auto coord = m_click_pos / gl::window_dimension();
        coord.y = 1.f - coord.y;
        auto px_length = 1.f / gl::window_dimension();

        const auto &control_points = quad_warp().control_points();

        for(uint32_t i = 0; i < control_points.size(); i++)
        {
            auto c = quad_warp().control_point(i);

            if(glm::length(c - coord) < 15 * glm::length(px_length))
            {
                if(!e.is_control_down())
                {
                    quad_warp().selected_indices().clear();
                    m_active_control_points.clear();
                }
                control_point_t cp(i, c);
                m_active_control_points.erase(cp);
                m_active_control_points.insert(cp);

                quad_warp().selected_indices().insert(i);
                LOG_TRACE_2 << "selected control point: " << i << " -> " << glm::to_string(coord);
            }
        }
    }else if(e.is_right())
    {
        m_active_control_points.clear();
        quad_warp().selected_indices().clear();
    }
}

void WarpComponent::mouse_move(const MouseEvent &e)
{
    m_mouse_pos = glm::vec2(e.get_x(), e.get_y());
}

void WarpComponent::mouse_drag(const MouseEvent &e)
{
    m_mouse_pos = glm::vec2(e.get_x(), e.get_y());
    glm::vec2 mouseDiff = m_mouse_pos - m_click_pos;

    auto inc = mouseDiff / gl::window_dimension();
    inc.y *= -1.f;

    for(auto cp : m_active_control_points)
    {
        quad_warp().set_control_point(cp.index, cp.value + inc);
    }
}

}// namespace kinski
