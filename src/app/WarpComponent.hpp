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

#include "core/Component.hpp"
#include "QuadWarp.hpp"

namespace kinski
{
    DEFINE_CLASS_PTR(WarpComponent);
    
    class KINSKI_API WarpComponent : public kinski::Component
    {
    public:
        
        WarpComponent();
        ~WarpComponent();
        
        void update_property(const Property::ConstPtr &the_property);
        
        void refresh();
        void reset();
        
        gl::QuadWarp& quad_warp(int i = -1);
        
        void render_output(int the_index, const gl::Texture &the_tex, const float the_brightness = 1.f);
        void set_from(gl::QuadWarp &the_quadwarp, uint32_t the_index = 0);
        uint32_t index() const{ return *m_index; }
        void set_index(int the_index) { *m_index = the_index; }
        void set_enabled(int the_index, bool b);
        bool enabled(int the_index);
        void set_display_grid(int the_index, bool b);
        bool display_grid(int the_index) const;
        void set_display_points(int the_index, bool b);
        bool display_points(int the_index) const;
        
        uint32_t num_warps() const;
        
    private:
        std::vector<gl::QuadWarp> m_quad_warp{10};
        
        typedef struct
        {
            bool enabled = false;
            bool display_grid = false;
            bool display_points = false;
        } param_t;
        
        std::vector<param_t> m_params;
        
        Property_<uint32_t>::Ptr m_index;
        Property_<uint32_t>::Ptr m_grid_sz_x, m_grid_sz_y;
        Property_<bool>::Ptr m_enabled, m_draw_grid, m_draw_control_points;
        
        Property_<gl::vec2>::Ptr m_src_top_left, m_src_bottom_right;
        Property_<gl::vec2>::Ptr m_top_left, m_top_right, m_bottom_left, m_bottom_right;
    };
}