// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  Noise.cpp
//
//  Created by Croc Dialer on 25/08/15.

#include "Noise.hpp"
#include "gl/Fbo.hpp"
#include "gl/Material.hpp"

namespace kinski{ namespace gl{

    struct NoiseImpl
    {
        gl::Fbo m_fbo;
        gl::MaterialPtr m_material;
        
        vec2 m_scale;
        ivec2 m_tex_size;
    };

    Noise::Noise(const vec2 &the_scale, const ivec2 &the_tex_size):
    m_impl(new NoiseImpl)
    {
        m_impl->m_scale = the_scale;
        m_impl->m_tex_size = the_tex_size;
    }
    
    gl::Texture Noise::simplex(const float the_seed)
    {
        gl::Texture noise_tex;
        
        if(!m_impl->m_fbo || m_impl->m_fbo.size() != m_impl->m_tex_size)
        {
            gl::Fbo::Format fmt;
#if !defined(KINSKI_GLES)
            fmt.set_color_internal_format(GL_R32F);
#endif
            m_impl->m_fbo = gl::Fbo(m_impl->m_tex_size, fmt);
        }
        if(!m_impl->m_material)
        {
            m_impl->m_material = gl::Material::create(gl::ShaderType::NOISE_3D);
            m_impl->m_material->set_depth_test(false);
            m_impl->m_material->set_depth_write(false);
        }
        m_impl->m_material->uniform("u_scale", m_impl->m_scale);
        m_impl->m_material->uniform("u_seed", the_seed);
        
        noise_tex = gl::render_to_texture(m_impl->m_fbo, [this]()
        {
            KINSKI_CHECK_GL_ERRORS();
            gl::draw_quad(m_impl->m_tex_size, m_impl->m_material);
        });
        KINSKI_CHECK_GL_ERRORS();
        return noise_tex;
    }
    
    ImagePtr Noise::create_simplex_image(const float the_seed)
    {
        int w = m_impl->m_tex_size.x, h = m_impl->m_tex_size.y;
        ImagePtr ret = Image::create(w, h, 1);
    
        for (int i = 0; i < h; ++i)
        {
            for (int j = 0; j < w; ++j)
            {
                ret->data[i * h + j] =
                255 * (glm::simplex(vec3(vec2(i, j) * m_impl->m_scale, the_seed)) + 1.f) / 2.f;
            }
        }
        return ret;
    }
    
    const ivec2& Noise::tex_size() const
    {
        return m_impl->m_tex_size;
    }
    
    const vec2& Noise::scale() const
    {
        return m_impl->m_scale;
    }
    
    void Noise::set_tex_size(const ivec2 &the_tex_size)
    {
        m_impl->m_tex_size = the_tex_size;
    }
    
    void Noise::set_scale(const vec2 &the_scale)
    {
        m_impl->m_scale = the_scale;
    }
}}
