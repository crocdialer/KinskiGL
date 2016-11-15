// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  Noise.hpp
//
//  Created by Croc Dialer on 25/08/15.

#pragma once

#include "core/Image.hpp"
#include "gl/gl.hpp"

namespace kinski{ namespace gl{

class KINSKI_API Noise
{
 public:
    
    Noise(const vec2 &the_scale = vec2(0.05f), const vec2 &the_tex_size = vec2(128));
    
    gl::Texture simplex(const float the_seed);
    
    ImagePtr create_simplex_image(const float the_seed);
    
    const vec2& tex_size() const;
    const vec2& scale() const;
    
    void set_tex_size(const vec2 &the_tex_size);
    void set_scale(const vec2 &the_scale);

 private:
    
    struct Impl;
    typedef std::shared_ptr<Impl> ImplPtr;
    ImplPtr m_impl;
};

}}// namespaces
