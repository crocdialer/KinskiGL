//
//  Material.h
//  kinskiGL
//
//  Created by Fabian on 10/12/12.
//
//

#ifndef __kinskiGL__Material__
#define __kinskiGL__Material__

#include <boost/variant.hpp>
#include "KinskiGL.h"
#include "Shader.h"
#include "Texture.h"

namespace kinski { namespace gl {
    
    class Material
    {
    public:
        
        typedef boost::variant<GLint, GLfloat, GLdouble, glm::vec2,
        glm::vec3, glm::vec4, glm::mat3, glm::mat4> UniformValue;
        
        typedef std::map<std::string, UniformValue> UniformMap;
     
    private:
        
        mutable Shader m_shader;
        UniformMap m_uniforms;
        
        std::vector<Texture> m_textures;
        
        glm::vec4 m_diffuse;
        glm::vec4 m_ambient;
        glm::vec4 m_specular;
        glm::vec4 m_emission;
        
        GLenum m_polygonMode;
        
        bool m_wireFrame;
        
        bool m_depthTest;
        
        bool m_depthWrite;
        
    public:
        
        Material(const Shader &theShader = Shader(), const UniformMap &theUniforms = UniformMap());
        
        void apply() const;
        
        void addTexture(const Texture &theTexture) {m_textures.push_back(theTexture);};
        
        void uniform(const std::string &theName, const UniformValue &theVal)
        { m_uniforms[theName] = theVal; };
        
        Shader& getShader() {return m_shader;};
        const Shader& getShader() const {return m_shader;};
        
        std::vector<Texture>& getTextures() {return m_textures;};
        const std::vector<Texture>& getTextures() const {return m_textures;};
        
        UniformMap& getUniforms() {return m_uniforms;};
        const UniformMap& getUniforms() const {return m_uniforms;};
        
        void setWireframe(bool b = true) { m_wireFrame = b;};
        
        void setDepthTest(bool b = true) { m_depthTest = b;};
        
        void setDepthWrite(bool b = true) { m_depthWrite = b;};
    };
   
}} // namespace

#endif 
