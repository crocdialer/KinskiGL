// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  QuadWarp.hpp
//
//  Created by Croc Dialer on 04/12/15.

#pragma once

#include "gl/gl.hpp"

namespace kinski{ namespace gl{
    
    class KINSKI_API QuadWarp
    {
    public:
        
        QuadWarp();

        void render_output(const gl::Texture &the_texture, const float the_brightness = 1.f);
        
        void render_control_points();
        
        void render_grid();
        
        ivec2 grid_resolution() const;
        void set_grid_resolution(const gl::ivec2 &the_res);
        void set_grid_resolution(uint32_t the_res_w, uint32_t the_res_h);
        
        void move_center_to(const gl::vec2 &the_pos);
        gl::vec2 center() const;
        
        const Area_<uint32_t>& src_area() const;
        void set_src_area(const Area_<uint32_t>& the_src_area);
        
//        const gl::vec2& control_point(int the_x, int the_y) const;
        gl::vec2& control_point(int the_x, int the_y);
        
        void set_control_point(int the_x, int the_y, const gl::vec2 &the_point);
        
        std::vector<gl::vec2>& control_points();
        
    private:
        
        struct Impl;
        std::shared_ptr<Impl> m_impl;
    };
    

}}// namespaces