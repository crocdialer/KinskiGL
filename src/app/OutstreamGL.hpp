// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  OutstreamGL.hpp
//
//  Created by Fabian on 5/13/13.

#pragma once

#include "gl/gl.hpp"
#include "gl/Font.hpp"
#include "gl/Fbo.hpp"

namespace kinski{ namespace gl{
    
    /* see
     * http://savingyoutime.wordpress.com/2009/04/21/using-c-stl-streambufostream-to-create-time-stamped-logging-class/
     * for references
     */
    
    class OutstreamGL : public std::ostream
    {
    public:
        
        explicit OutstreamGL(uint32_t max_lines = 11);
        explicit OutstreamGL(const gl::Font &the_font, uint32_t max_lines = 10);
        virtual ~OutstreamGL();
        
        const std::list<std::string>& lines() const {return m_lines;};
        uint32_t max_lines() const {return m_max_lines;}
        void set_max_lines(uint32_t ml){m_max_lines = ml;}
        
        const gl::Font& font() const {return m_font;};
        gl::Font& font() {return m_font;};
        void set_font(gl::Font &the_font){m_font = the_font;};
        
        const gl::Color& color() const {return m_color;}
        void set_color(const gl::Color &c){m_color = c;}
        
        void add_line(const std::string &line);
        void draw();
    
    private:
        
        gl::Font m_font;
        gl::Color m_color;
        uint32_t m_max_lines;
        std::list<std::string> m_lines;
        gl::Fbo m_fbo;
        gl::ScenePtr m_gui_scene;
        gl::MaterialPtr m_blend_material;
        bool m_dirty;
    };
}}//namespace
