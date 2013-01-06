#ifndef _KINSKIGL_H
#define _KINSKIGL_H

#include "kinskiCore/Definitions.h"
#include "kinskiCore/Exception.h"
#include "kinskiCore/Logger.h"
#include "kinskiCore/file_functions.h"

//triggers checks with glGetError()
#define KINSKI_GL_REPORT_ERRORS


#if defined(KINSKI_COCOA_TOUCH) || defined(KINSKI_RASPI)
#define KINSKI_GLES
#endif

#if defined(KINSKI_RASPI)
#define KINSKI_NO_VAO
#endif

#ifdef KINSKI_GLES // GLES2
#ifdef KINSKI_COCOA_TOUCH
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

#else // general
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif

#elif defined(KINSKI_COCOA)// desktop GL3

#include <OpenGL/gl3.h>
#else
#include <GL3/gl3.h>
#endif

// crossplattform helper-macros to append either nothing or "OES"
// appropriately to a symbol based on either OpenGL3 or OpenGLES2
#if defined( KINSKI_GLES )
#define GL_SUFFIX(sym) sym##OES
#define GL_ENUM(sym) sym##_OES
#else
#define GL_SUFFIX(sym) sym
#define GL_ENUM(sym) sym
#endif

#ifdef KINSKI_GL_REPORT_ERRORS
#define KINSKI_CHECK_GL_ERRORS()\
while(GLenum error = glGetError()){\
switch(error){\
case GL_INVALID_ENUM:\
LOG_ERROR << "GL_INVALID_ENUM"; break;\
case GL_INVALID_VALUE:\
LOG_ERROR << "GL_INVALID_VALUE"; break;\
case GL_INVALID_OPERATION:\
LOG_ERROR << "GL_INVALID_OPERATION"; break;\
case GL_INVALID_FRAMEBUFFER_OPERATION:\
LOG_ERROR << "GL_INVALID_FRAMEBUFFER_OPERATION"; break;\
case GL_OUT_OF_MEMORY:\
LOG_ERROR << "GL_OUT_OF_MEMORY"; break;\
default:\
LOG_ERROR << "Unknown GLerror"; break;\
}\
}
#else
#define KINSKI_CHECK_GL_ERRORS()
#endif

#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtc/random.hpp>

namespace kinski { namespace gl {

    // forward declarations
    class Buffer;
    class Texture;
    class Material;
    class Shader;
    class Geometry;
    class Mesh;
    struct Animation;
    struct Bone;
    
    enum Matrixtype { MODEL_VIEW_MATRIX = 1 << 0, PROJECTION_MATRIX = 1 << 1};
    void pushMatrix(const Matrixtype type);
    void popMatrix(const Matrixtype type);
    void multMatrix(const glm::mat4 &theMatrix);
    void loadMatrix(const Matrixtype type, const glm::mat4 &theMatrix);
    void getMatrix(const Matrixtype type, glm::mat4 &theMatrix);
    
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
    
    /********************************* Drawing Functions *****************************************/
    
    void drawLine(const glm::vec2 &a, const glm::vec2 &b, const glm::vec4 &theColor = glm::vec4(1));
    
    void drawLines(const std::vector<glm::vec3> &thePoints, const glm::vec4 &theColor);
    
    void drawPoints(GLuint thePointVBO, GLsizei theCount,
                    const std::shared_ptr<Material> &theMaterial = std::shared_ptr<Material>(),
                    GLsizei stride = 0,
                    GLsizei offset = 0);
    
    void drawPoints(const std::vector<glm::vec3> &thePoints,
                    const std::shared_ptr<Material> &theMaterial = std::shared_ptr<Material>());
    
    void drawTexture(gl::Texture &theTexture, const glm::vec2 &theSize,
                     const glm::vec2 &theTopLeft = glm::vec2(0));
    
    void drawQuad(gl::Material &theMaterial, const glm::vec2 &theSize,
                  const glm::vec2 &theTopLeft = glm::vec2(0));
    
    void drawQuad(gl::Material &theMaterial, float x0, float y0, float x1, float y1);
    
    void drawGrid(float width, float height, int numW = 20, int numH = 20);
    
    void drawAxes(const std::weak_ptr<const Mesh> &theMesh);
    
    void drawMesh(const std::shared_ptr<const Mesh> &theMesh);
    
    void drawBoundingBox(const std::weak_ptr<const Mesh> &theMesh);
    
    void drawNormals(const std::weak_ptr<const Mesh> &theMesh);
        
    /*********************************** Shader Factory *******************************************/
    
    enum ShaderType {SHADER_UNLIT, SHADER_PHONG, SHADER_PHONG_SKIN};
    Shader createShader(ShaderType type);

}}//namespace

#endif //_KINSKIGL_H
