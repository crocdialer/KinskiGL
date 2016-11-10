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
    class KINSKI_API WarpComponent : public kinski::Component
    {
    public:
        typedef std::shared_ptr<WarpComponent> Ptr;
        
        WarpComponent();
        ~WarpComponent();
        
        void update_property(const Property::ConstPtr &the_property);
        
        void refresh();
        void reset();
        
        gl::QuadWarp& quad_warp(){ return m_quad_warp[*m_index]; }
        
        void render_output(const gl::Texture &the_tex, const float the_brightness = 1.f);
        
    private:
        std::vector<gl::QuadWarp> m_quad_warp{10};
        
        Property_<uint32_t>::Ptr m_index;
        Property_<uint32_t>::Ptr m_grid_sz_x, m_grid_sz_y;
        Property_<bool>::Ptr m_draw_grid, m_draw_control_points;
        
        Property_<gl::vec2>::Ptr m_top_left, m_top_right, m_bottom_left, m_bottom_right;
    };
}
