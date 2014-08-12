//
//  DrawHelper.h
//  gl
//
//  Created by Croc Dialer on 24/07/14.
//
//

#ifndef __gl__DrawHelper__
#define __gl__DrawHelper__

#include "gl/KinskiGL.h"

namespace kinski{ namespace gl{

    class DrawHelper
    {
        static int s_max_lvl;
    public:
        
        static void set_max_lvl(int the_lvl);
        static int max_lvl();
        static void draw_color_overlay(int lvl);
        
        static gl::Texture create_mask(int lvl);
        static gl::Texture create_empty_tex();
    };

    
}}

#endif /* defined(__gl__DrawHelper__) */
