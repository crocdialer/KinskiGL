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

namespace kinski { namespace gl {
    
    Material::Material(const Shader &theShader, const UniformMap &theUniforms):
    m_shader(theShader),
    m_uniforms(theUniforms),
    m_diffuse(Color(1)),
    m_ambient(Color(0)),
    m_specular(Color(1)),
    m_emission(Color(0)),
    m_shinyness(10.0f),
    m_polygonMode(GL_FRONT),
    m_twoSided(false),
    m_wireFrame(false),
    m_depthTest(true),
    m_depthWrite(true),
    m_blending(false), m_blendSrc(GL_SRC_ALPHA), m_blendDst(GL_ONE_MINUS_SRC_ALPHA),
    m_pointSize(1.f)
    {
        m_uniforms["u_material.diffuse"] = m_diffuse;
        m_uniforms["u_material.ambient"] = m_ambient;
        m_uniforms["u_material.specular"] = m_specular;
        m_uniforms["u_material.emmission"] = m_emission;
        m_uniforms["u_material.shinyness"] = m_shinyness;
        setPointAttenuation(1.f, 0.f, 0.f);
        m_shader = theShader? theShader : gl::createShader(gl::SHADER_UNLIT);
    }

    void Material::setDiffuse(const Color &theColor)
    {
        m_diffuse = glm::clamp(theColor, glm::vec4(0), glm::vec4(1));
        m_uniforms["u_material.diffuse"] = m_diffuse;
    }
    
    void Material::setAmbient(const Color &theColor)
    {
        m_ambient = glm::clamp(theColor, glm::vec4(0), glm::vec4(1));
        m_uniforms["u_material.ambient"] = m_ambient;
    }
    
    void Material::setSpecular(const Color &theColor)
    {
        m_specular = glm::clamp(theColor, glm::vec4(0), glm::vec4(1));
        m_uniforms["u_material.specular"] = m_specular;
    }
    
    void Material::setEmission(const Color &theColor)
    {
        m_emission = glm::clamp(theColor, glm::vec4(0), glm::vec4(1));
        m_uniforms["u_material.emmission"] = m_emission;
    }
    
    void Material::setShinyness(float s)
    {
        m_shinyness = s;
        m_uniforms["u_material.shinyness"] = s;
    }
    
    void Material::setPointAttenuation(float constant, float linear, float quadratic)
    {
        m_point_attenuation = PointAttenuation(constant, linear, quadratic);
        m_uniforms["u_point_attenuation.constant"] = constant;
        m_uniforms["u_point_attenuation.linear"] = linear;
        m_uniforms["u_point_attenuation.quadratic"] = quadratic;
    };

}}// namespace