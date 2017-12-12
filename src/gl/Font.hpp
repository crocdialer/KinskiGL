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
#include "core/Image.hpp"

namespace kinski { namespace gl {
    
class KINSKI_API Font
{
public:

    enum class Align{LEFT, CENTER, RIGHT};

    Font();

    void load(const std::string &the_path, size_t the_size, bool use_sdf = false);
    const std::string path() const;
    Texture glyph_texture() const;
    Texture sdf_texture() const;

    gl::AABB create_aabb(const std::string &theText) const;

    ImagePtr create_image(const std::string &theText, const vec4 &theColor = vec4(1)) const;
    Texture create_texture(const std::string &theText, const vec4 &theColor = vec4(1)) const;
    gl::MeshPtr create_mesh(const std::string &theText, const vec4 &theColor = vec4(1)) const;

    gl::Object3DPtr create_text_obj(const std::string &the_text,
                                    Align the_align = Align::LEFT,
                                    uint32_t the_linewidth = 0,
                                    uint32_t the_lineheight = 0) const;

    gl::Object3DPtr create_text_obj(std::list<std::string> the_lines,
                                    Align the_align = Align::LEFT,
                                    uint32_t the_linewidth = 0,
                                    uint32_t the_lineheight = 0) const;

    template<template<typename> class Collection, typename T = std::string>
    gl::Object3DPtr create_text_obj(const Collection<T> &the_lines,
                                    Align the_align = Align::LEFT,
                                    uint32_t the_linewidth = 0,
                                    uint32_t the_lineheight = 0) const
    {
        return create_text_obj(std::list<std::string>(std::begin(the_lines),
                                                      std::end(the_lines)),
                               the_align, the_linewidth, the_lineheight);
    }

    uint32_t font_size() const;
    uint32_t line_height() const;
    void set_line_height(uint32_t the_line_height);

private:
    std::shared_ptr<struct FontImpl> m_impl;

public:
    explicit operator bool() const { return m_impl.get(); }
    void reset() { m_impl.reset(); }
};

}}// namespace
