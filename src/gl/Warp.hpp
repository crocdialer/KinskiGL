// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  Warp.hpp
//
//  Created by Croc Dialer on 04/12/15.

#pragma once

#include "gl/gl.hpp"

namespace kinski{ namespace gl{
    
    class KINSKI_API Warp
    {
    public:
        
        static const gl::ivec2 s_max_num_subdivisions;
        
        Warp();

        void render_output(const gl::Texture &the_texture, const float the_brightness = 1.f);
        
        void render_control_points();
        
        void render_grid();
        
        ivec2 grid_resolution() const;
        void set_grid_resolution(const gl::ivec2 &the_res);
        void set_grid_resolution(uint32_t the_res_w, uint32_t the_res_h);
        
        ivec2 num_subdivisions() const;
        void set_num_subdivisions(const gl::ivec2 &the_res);
        void set_num_subdivisions(uint32_t the_div_w, uint32_t the_div_h);
        
        void move_center_to(const gl::vec2 &the_pos);
        gl::vec2 center() const;
        
        void flip_content(bool horizontal = true);
        void rotate_content(bool clock_wise = true);
        
        const Area_<uint32_t>& src_area() const;
        void set_src_area(const Area_<uint32_t>& the_src_area);
        
        const gl::vec2 control_point(int the_x, int the_y) const;
        const gl::vec2 control_point(int the_index) const;
        
        void set_control_point(int the_x, int the_y, const gl::vec2 &the_point);
        void set_control_point(int the_index, const gl::vec2 &the_point);
        
        const std::vector<gl::vec2>& control_points() const;
        void set_control_points(const std::vector<gl::vec2> &cp);
        
        bool cubic_interpolation() const;
        void set_cubic_interpolation(bool b);
        
        std::set<uint32_t>& selected_indices();
        
        const std::vector<gl::vec2>& corners() const;
        void set_corners(const std::vector<gl::vec2> &cp);
        
        const gl::mat4& transform() const;
        
        void reset();
        
    private:
        
        std::shared_ptr<struct Impl> m_impl;
    };
    

}}// namespaces
