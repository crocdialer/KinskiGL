//
//  DrawHelper.cpp
//  gl
//
//  Created by Croc Dialer on 24/07/14.
//
//

#include "DrawHelper.h"
#include "gl/Texture.h"

namespace kinski{ namespace gl{

    int DrawHelper::s_max_lvl = 6;
    
    void DrawHelper::draw_color_overlay(int lvl)
    {
        std::vector<gl::Color> colors =
        {
            gl::COLOR_RED,
            gl::COLOR_DARK_RED,
            gl::COLOR_PURPLE,
            gl::COLOR_BLUE,
            gl::COLOR_OLIVE,
            gl::COLOR_ORANGE,
            gl::COLOR_WHITE
        };
        
        glm::vec2 win_sz = windowDimension();
        glm::vec2 offset(0), step(0.f, win_sz.y / (float) colors.size());
        
        for(auto &c : colors){c.a = .7;}
        
        for(int i = 0; i < colors.size(); i++)
        {
            gl::drawQuad(colors[i], win_sz, offset);
            offset += step;
        }
    }
    
    void DrawHelper::set_max_lvl(int the_lvl)
    {
    
    }
    
    int DrawHelper::max_lvl()
    {
        return s_max_lvl;
    }
    
    gl::Texture DrawHelper::create_mask(int lvl)
    {
        gl::Texture ret;
        
        int px_height = 2 * lvl;
        int px_total = 2 * s_max_lvl;
        
        uint8_t data[px_total];
        
        for (int i = 0 ; i < px_total; i++)
        {
            data[i] = i > px_height ? 0 : 255;
        }
        ret.update(data, GL_RED, 1, px_total, true);
        
        return ret;
    }
    
    gl::Texture DrawHelper::create_empty_tex()
    {
        gl::Texture ret;
        uint8_t data[1] = {0};
        ret.update(data, GL_RED, 1, 1, true);
        return ret;
    }
    
}}// namespaces
