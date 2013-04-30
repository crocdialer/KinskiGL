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
    m_diffuse(glm::vec4(1)),
    m_ambient(glm::vec4(0)),
    m_specular(glm::vec4(1)),
    m_emission(glm::vec4(0)),
    m_shinyness(10.0f),
    m_polygonMode(GL_FRONT),
    m_twoSided(false),
    m_wireFrame(false),
    m_depthTest(true),
    m_depthWrite(true),
    m_blending(false), m_blendSrc(GL_SRC_ALPHA), m_blendDst(GL_ONE_MINUS_SRC_ALPHA),
    m_pointSize(0.f)
    {
        m_uniforms["u_material.diffuse"] = m_diffuse;
        m_uniforms["u_material.ambient"] = m_ambient;
        m_uniforms["u_material.specular"] = m_specular;
        m_uniforms["u_material.emmission"] = m_emission;
        m_uniforms["u_material.shinyness"] = m_shinyness;
        m_shader = theShader? theShader : gl::createShader(gl::SHADER_UNLIT);
    }

    void Material::setDiffuse(const glm::vec4 &theColor)
    {
        m_diffuse = glm::clamp(theColor, glm::vec4(0), glm::vec4(1));
        m_uniforms["u_material.diffuse"] = m_diffuse;
    }
    
    void Material::setAmbient(const glm::vec4 &theColor)
    {
        m_ambient = glm::clamp(theColor, glm::vec4(0), glm::vec4(1));
        m_uniforms["u_material.ambient"] = m_ambient;
    }
    
    void Material::setSpecular(const glm::vec4 &theColor)
    {
        m_specular = glm::clamp(theColor, glm::vec4(0), glm::vec4(1));
        m_uniforms["u_material.specular"] = m_specular;
    }
    
    void Material::setEmission(const glm::vec4 &theColor)
    {
        m_emission = glm::clamp(theColor, glm::vec4(0), glm::vec4(1));
        m_uniforms["u_material.emmission"] = m_emission;
    }
    
    void Material::setShinyness(float s)
    {
        m_shinyness = s;
        m_uniforms["u_material.shinyness"] = s;
    }

}}// namespace