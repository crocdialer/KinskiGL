//
//  QuadWarp.h
//  kinskiGL
//
//  Created by Croc Dialer on 04/12/15.
//
//

#pragma once

#include "gl/gl.hpp"

namespace kinski{ namespace gl{
    
    class KINSKI_API QuadWarp
    {
    public:
        
        QuadWarp();

        void render_output(const gl::Texture &the_texture);
        
        void render_control_points();
        
        void render_grid();
        
        ivec2 grid_resolution() const;
        void set_grid_resolution(const gl::ivec2 &the_res);
        void set_grid_resolution(uint32_t the_res_w, uint32_t the_res_h);
        
//        const gl::vec2& control_point(int the_x, int the_y) const;
        gl::vec2& control_point(int the_x, int the_y);
        
        void set_control_point(int the_x, int the_y, const gl::vec2 &the_point);
        
    private:
        
        struct Impl;
        std::shared_ptr<Impl> m_impl;
    };
    

}}// namespaces