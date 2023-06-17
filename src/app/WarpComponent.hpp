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
//  Created by Fabian on 12/05/15.

#pragma once

#include "Component.hpp"

#include "gl/Font.hpp"
#include "Warp.hpp"

namespace kinski {
DEFINE_CLASS_PTR(WarpComponent);

class KeyEvent;

class MouseEvent;

class WarpComponent : public crocore::Component
{
public:

    static WarpComponentPtr create();

    void update_property(const crocore::PropertyConstPtr &the_property);

    void refresh();

    void reset();

    gl::Warp &quad_warp(int i = -1);

    void render_output(int the_index, const gl::Texture &the_tex, const float the_brightness = 1.f);

    void set_from(gl::Warp &the_quadwarp, uint32_t the_index = 0);

    uint32_t index() const { return *m_index; }

    void set_index(int the_index) { *m_index = the_index; }

    void set_enabled(int the_index, bool b);

    bool enabled(int the_index);

    void set_display_grid(int the_index, bool b);

    bool display_grid(int the_index) const;

    void set_display_points(int the_index, bool b);

    bool display_points(int the_index) const;

    uint32_t num_warps() const;

    void set_font(const gl::Font &the_font) { m_font = the_font; };

    void key_press(const KeyEvent &e);

    void key_release(const KeyEvent &e);

    void mouse_press(const MouseEvent &e);

    void mouse_move(const MouseEvent &e);

    void mouse_drag(const MouseEvent &e);

private:
    WarpComponent();

    std::vector<gl::Warp> m_quad_warp{10};

    typedef struct
    {
        bool enabled = false;
        bool display_grid = false;
        bool display_points = false;
    }param_t;

    std::vector<param_t> m_params;

    struct control_point_t
    {
        int index = -1;
        gl::vec2 value;

        control_point_t(int the_index, gl::vec2 the_value) : index(the_index), value(the_value) {}

        bool operator<(const control_point_t &the_other) const { return index < the_other.index; }
    };

    std::set<control_point_t> m_active_control_points;
    gl::vec2 m_click_pos, m_mouse_pos;
    bool m_show_cursor = false;
    gl::Font m_font;

    crocore::Property_<int>::Ptr m_index;
    crocore::Property_<gl::ivec2>::Ptr m_num_subdivisions;
    crocore::Property_<gl::ivec2>::Ptr m_grid_resolution;
    crocore::Property_<bool>::Ptr m_enabled, m_draw_grid, m_draw_control_points, m_perspective,
            m_cubic_interpolation;

    crocore::Property_<gl::vec2>::Ptr m_src_top_left, m_src_size;
    crocore::Property_<std::vector<gl::vec2>>::Ptr m_corners, m_control_points;
    crocore::Property_<std::vector<float>>::Ptr m_edges, m_edge_exponents;
};
}
