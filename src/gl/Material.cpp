// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Material.h"

using namespace std;
using namespace glm;

namespace kinski { namespace gl {
    
    Material::Material(const Shader &theShader, const UniformMap &theUniforms):
    m_shader(theShader),
    m_uniforms(theUniforms),
    m_dirty(true),
    m_polygonMode(GL_FRONT),
    m_twoSided(false),
    m_wireFrame(false),
    m_depthTest(true),
    m_depthWrite(true),
    m_blending(false),
    m_blend_src(GL_SRC_ALPHA), m_blend_dst(GL_ONE_MINUS_SRC_ALPHA),
    m_blend_equation(GL_FUNC_ADD),
    m_diffuse(Color(1)),
    m_ambient(Color(1)),
    m_specular(Color(1)),
    m_emission(Color(0)),
    m_shinyness(10.0f),
    m_pointSize(1.f)
    {
        setPointAttenuation(1.f, 0.f, 0.f);
        m_shader = theShader? theShader : gl::createShader(gl::SHADER_UNLIT);
    }

    void Material::setDiffuse(const Color &theColor)
    {
        m_diffuse = glm::clamp(theColor, glm::vec4(0), glm::vec4(1));
        m_dirty = true;
    }
    
    void Material::setAmbient(const Color &theColor)
    {
        m_ambient = glm::clamp(theColor, glm::vec4(0), glm::vec4(1));
        m_dirty = true;
    }
    
    void Material::setSpecular(const Color &theColor)
    {
        m_specular = glm::clamp(theColor, glm::vec4(0), glm::vec4(1));
        m_dirty = true;
    }
    
    void Material::setEmission(const Color &theColor)
    {
        m_emission = glm::clamp(theColor, glm::vec4(0), glm::vec4(1));
        m_dirty = true;
    }
    
    void Material::setShinyness(float s)
    {
        m_shinyness = s;
        m_dirty = true;
    }
    
    void Material::setPointSize(float sz)
    {
        m_pointSize = sz;
        m_dirty = true;
    }
    
    void Material::setPointAttenuation(float constant, float linear, float quadratic)
    {
        m_point_attenuation = PointAttenuation(constant, linear, quadratic);
        m_dirty = true;
    };
    
    void Material::update_uniform_buffer()
    {
        
#if !defined(KINSKI_GLES)
        if(!m_uniform_buffer){ m_uniform_buffer = gl::Buffer(GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW); }
        
        
        struct material_struct_std140
        {
            vec4 diffuse;
            vec4 ambient;
            vec4 specular;
            vec4 emission;
            float shinyness;
            float point_size;
            PointAttenuation point_attenuation;// 3 floats
        };
        
        if(m_dirty)
        {
            material_struct_std140 m;
            m.diffuse = m_diffuse;
            m.ambient = m_ambient;
            m.specular = m_specular;
            m.emission = m_emission;
            m.shinyness = m_shinyness;
            m.point_size = m_pointSize;
            m.point_attenuation = m_point_attenuation;
            
            m_uniform_buffer.setData(&m, sizeof(m));
            m_dirty = false;
        }
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_uniform_buffer.id());
#else
        if(m_dirty)
        {
            m_uniforms["u_material.diffuse"] = m_diffuse;
            m_uniforms["u_material.ambient"] = m_ambient;
            m_uniforms["u_material.specular"] = m_specular;
            m_uniforms["u_material.emmission"] = m_emission;
            m_uniforms["u_material.shinyness"] = m_shinyness;
            m_uniforms["u_pointSize"] = m_pointSize;
            m_uniforms["u_material.point_attenuation.constant"] = m_point_attenuation.constant;
            m_uniforms["u_material.point_attenuation.linear"] = m_point_attenuation.linear;
            m_uniforms["u_material.point_attenuation.quadratic"] = m_point_attenuation.quadratic;
            m_dirty = false;
        }
#endif
    }
    
}}// namespace