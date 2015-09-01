//
//  Noise.cpp
//  kinskiGL
//
//  Created by Croc Dialer on 25/08/15.
//
//

#include "Noise.h"
#include "gl/Fbo.h"
#include "gl/Material.h"

namespace kinski{ namespace gl{

    struct Noise::Impl
    {
        gl::Fbo m_fbo;
        gl::MaterialPtr m_material;
        
        vec2 m_scale, m_tex_size;
    };

    Noise::Noise(const vec2 &the_scale, const vec2 &the_tex_size):
    m_impl(new Impl)
    {
        m_impl->m_scale = the_scale;
        m_impl->m_tex_size = the_tex_size;
    }
    
    gl::Texture Noise::simplex(const float the_seed)
    {
        gl::Texture noise_tex;
        
        if(!m_impl->m_fbo || m_impl->m_fbo.getSize() != m_impl->m_tex_size)
        {
            m_impl->m_fbo = gl::Fbo(m_impl->m_tex_size);
        }
        if(!m_impl->m_material)
        {
            m_impl->m_material = gl::Material::create(gl::createShader(gl::SHADER_NOISE_3D));
            m_impl->m_material->setDepthTest(false);
            m_impl->m_material->setDepthWrite(false);
        }
        m_impl->m_material->uniform("u_scale", m_impl->m_scale);
        m_impl->m_material->uniform("u_seed", the_seed);
        
        noise_tex = gl::render_to_texture(m_impl->m_fbo, [this]()
        {
            gl::drawQuad(m_impl->m_material, m_impl->m_tex_size);
        });
        return noise_tex;
    }
    
    const vec2& Noise::tex_size() const
    {
        return m_impl->m_tex_size;
    }
    
    const vec2& Noise::scale() const
    {
        return m_impl->m_scale;
    }
    
    void Noise::set_tex_size(const vec2 &the_tex_size)
    {
        m_impl->m_tex_size = the_tex_size;
    }
    
    void Noise::set_scale(const vec2 &the_scale)
    {
        m_impl->m_scale = the_scale;
    }
}}