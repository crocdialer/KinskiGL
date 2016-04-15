// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  Font.h
//
//  Created by Fabian Schmidt on 3/9/13.

#pragma once

#include "gl/gl.hpp"

namespace kinski { namespace gl {
    
    class KINSKI_API Font
    {
    public:
        
        enum class Align{LEFT, CENTER, RIGHT};
        
        Font();
        
        void load(const std::string &the_path, size_t the_size, size_t line_height = 0);
        const std::string path() const;
        Texture glyph_texture() const;
        
        Texture create_texture(const std::string &theText, const vec4 &theColor = vec4(1)) const;
        gl::MeshPtr create_mesh(const std::string &theText, const vec4 &theColor = vec4(1)) const;
        gl::Object3DPtr create_text_obj(const std::string &the_text, float the_linewidth,
                                        Align the_align = Align::LEFT) const;
        uint32_t font_size() const;
        uint32_t line_height() const;
        
    private:
        // forward declared Implementation object
        struct Obj;
        typedef std::shared_ptr<Obj> ObjPtr;
        ObjPtr m_obj;
        
    public:
        //! Emulates shared_ptr-like behavior
        operator bool() const { return m_obj.get() != nullptr; }
        void reset() { m_obj.reset(); }
    };
    
}}// namespace