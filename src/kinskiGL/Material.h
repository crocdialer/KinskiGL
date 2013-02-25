// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef __kinskiGL__Material__
#define __kinskiGL__Material__

#include <boost/variant.hpp>
#include "KinskiGL.h"
#include "Shader.h"
#include "Texture.h"

namespace kinski { namespace gl {
    
    class KINSKI_API Material
    {
    public:
        
        typedef std::shared_ptr<Material> Ptr;
        typedef std::shared_ptr<const Material> ConstPtr;
        
        typedef boost::variant<GLint, GLfloat, double, glm::vec2, glm::vec3, glm::vec4,
        glm::mat3, glm::mat4,
        std::vector<GLint>, std::vector<GLfloat>,
        std::vector<glm::vec2>, std::vector<glm::vec3>, std::vector<glm::vec4>,
        std::vector<glm::mat3>, std::vector<glm::mat4> > UniformValue;
        
        typedef std::map<std::string, UniformValue> UniformMap;

        Material(const Shader &theShader = Shader(), const UniformMap &theUniforms = UniformMap());
        
        void apply() const;
        
        void addTexture(const Texture &theTexture) {m_textures.push_back(theTexture);};
        
        void uniform(const std::string &theName, const UniformValue &theVal)
        { m_uniforms[theName] = theVal; };
        
        Shader& shader() {return m_shader;};
        const Shader& shader() const {return m_shader;};
        
        void setShader(const Shader &theShader) { m_shader = theShader; };
        
        std::vector<Texture>& textures() {return m_textures;};
        const std::vector<Texture>& textures() const {return m_textures;};
        
        UniformMap& uniforms() {return m_uniforms;};
        const UniformMap& uniforms() const {return m_uniforms;};
        
        void setTwoSided(bool b = true) { m_twoSided = b;};
        
        bool twoSided() const { return m_twoSided; };
        
        void setWireframe(bool b = true) { m_wireFrame = b;};
        
        void setDepthTest(bool b = true) { m_depthTest = b;};
        
        void setDepthWrite(bool b = true) { m_depthWrite = b;};
        
        void setBlending(bool b = true) { m_blending = b;};
        
        bool opaque() const { return m_diffuse.a == 1.f ;};
        
        const glm::vec4& diffuse() const { return m_diffuse; };
        const glm::vec4& ambient() const { return m_ambient; };
        const glm::vec4& specular() const { return m_specular; };
        const glm::vec4& emission() const { return m_emission; };
        const float shinyness() const { return m_shinyness; };
        
        void setDiffuse(const glm::vec4 &theColor);
        void setAmbient(const glm::vec4 &theColor);
        void setSpecular(const glm::vec4 &theColor);
        void setEmission(const glm::vec4 &theColor);
        void setShinyness(float s);
        void setPointSize(float sz){ m_pointSize = sz; };
        
    private:
        
        mutable Shader m_shader;
        UniformMap m_uniforms;
        
        std::vector<Texture> m_textures;
        
        glm::vec4 m_diffuse;
        glm::vec4 m_ambient;
        glm::vec4 m_specular;
        glm::vec4 m_emission;
        float m_shinyness;
        
        GLenum m_polygonMode;
        
        bool m_twoSided;
        bool m_wireFrame;
        bool m_depthTest;
        bool m_depthWrite;
        bool m_blending;
        
        GLenum m_blendSrc, m_blendDst;
        
        float m_pointSize;
    };
   
}} // namespace

#endif 
