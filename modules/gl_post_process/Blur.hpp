// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  Blur.hpp
//
//  Created by Croc Dialer on 12/06/16.

#pragma once

#include "gl/gl.hpp"

namespace kinski{ namespace gl{
    
    class KINSKI_API Blur
    {
    public:
        
        Blur(const glm::vec2 &the_scale = glm::vec2(3.f));
        
        void render_output(const gl::Texture &the_texture);

        glm::vec2 scale() const;
        void set_scale(const glm::vec2 &the_scale);
        
    private:

        std::shared_ptr<struct BlurImpl> m_impl;
    };
    
    
}}// namespaces