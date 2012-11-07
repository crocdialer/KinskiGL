#ifndef _KINSKIGL_H
#define _KINSKIGL_H

//#define KINSKI_GLES
#define GLFW_INCLUDE_GL3
#define GLFW_NO_GLU

#include <GL/glfw.h>
#include <AntTweakBar.h>

#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/noise.hpp>

#include "kinskiCore/Definitions.h"
#include "kinskiCore/Exception.h"


namespace kinski { namespace gl {

    class Texture;
    class Material;
    
    void drawLine(const glm::vec2 &a, const glm::vec2 &b, const glm::vec4 &theColor = glm::vec4(1));
    
    void drawTexture(gl::Texture &theTexture, const glm::vec2 &theSize,
                     const glm::vec2 &theTopLeft = glm::vec2(0));
    
    void drawQuad(gl::Material &theMaterial, const glm::vec2 &theSize,
                  const glm::vec2 &theTopLeft = glm::vec2(0));
    
    void drawQuad(gl::Material &theMaterial, float x0, float y0, float x1, float y1);
    
}}//namespace

#endif //_KINSKIGL_H