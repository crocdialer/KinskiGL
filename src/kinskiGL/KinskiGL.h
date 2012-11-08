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
    
    enum Matrixtype { MODEL_VIEW_MATRIX = 1 << 0, PROJECTION_MATRIX = 1 << 1};
    void pushMatrix(const Matrixtype type);
    void popMatrix(const Matrixtype type);
    void multMatrix(const glm::mat4 &theMatrix);
    void loadMatrix(const Matrixtype type, const glm::mat4 &theMatrix);
    
    class ScopedMatrixPush
    {
    public:
        ScopedMatrixPush(const Matrixtype type):
        m_type(type)
        {pushMatrix(type);}
        
        ~ScopedMatrixPush()
        {popMatrix(m_type);}
    private:
        Matrixtype m_type;
    };
    
    void setWindowDimension(const glm::vec2 &theDim);
    
    void drawLine(const glm::vec2 &a, const glm::vec2 &b, const glm::vec4 &theColor = glm::vec4(1));
    
    void drawLines(const std::vector<glm::vec3> &thePoints, const glm::vec4 &theColor);
    
    void drawPoints(const std::vector<glm::vec3> &thePoints);
    
    void drawTexture(gl::Texture &theTexture, const glm::vec2 &theSize,
                     const glm::vec2 &theTopLeft = glm::vec2(0));
    
    void drawQuad(gl::Material &theMaterial, const glm::vec2 &theSize,
                  const glm::vec2 &theTopLeft = glm::vec2(0));
    
    void drawQuad(gl::Material &theMaterial, float x0, float y0, float x1, float y1);
    
}}//namespace

#endif //_KINSKIGL_H