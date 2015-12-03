//
//  Noise.h
//  kinskiGL
//
//  Created by Croc Dialer on 25/08/15.
//
//

#ifndef __kinskiGL__QuadWarp__
#define __kinskiGL__QuadWarp__

#include "gl/gl.h"

namespace kinski{ namespace gl{
    
    class KINSKI_API QuadWarp
    {
    public:
        
        QuadWarp();
        
        void init();
        
        void render_output(gl::Texture &the_texture);
        
        void render_control_points();
        
        void render_grid();
        
        uint32_t grid_num_w() const;
        uint32_t grid_num_h() const;
        
        const gl::vec2& control_point(int the_x, int the_y) const;
        
        gl::vec2& control_point(int the_x, int the_y);
        
        void set_control_point(int the_x, int the_y, const gl::vec2 &the_point);
        
    private:
        
        struct Impl;
        std::shared_ptr<Impl> m_impl;
    };
    

}}// namespaces

#endif /* defined(__kinskiGL__Noise__) */
