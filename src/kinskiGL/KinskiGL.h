// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef _KINSKIGL_H
#define _KINSKIGL_H

#include "kinskiCore/Definitions.h"
#include "kinskiCore/Exception.h"
#include "kinskiCore/Logger.h"
#include "kinskiCore/file_functions.h"

//triggers checks with glGetError()
//#define KINSKI_GL_REPORT_ERRORS

#if defined(KINSKI_COCOA_TOUCH) || defined(KINSKI_RASPI)
#define KINSKI_GLES
#endif

#if defined(KINSKI_RASPI)
#define KINSKI_NO_VAO
#define GL_GLEXT_PROTOTYPES
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
#include <OpenGL/gl3ext.h>
#else
#define GLCOREARB_PROTOTYPES
#include <GL/glcorearb.h>
#include <GL/glext.h>
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

#define GLM_SWIZZLE
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/noise.hpp"
#include "glm/gtc/random.hpp"
#include "glm/gtx/norm.hpp"
#include "glm/gtx/string_cast.hpp"

#include "SerializerGL.h"

namespace kinski { namespace gl {

    // forward declarations
    class Buffer;
    class Shader;
    class Fbo;
    class Font;
    class Visitor;
    struct Ray;
    struct AABB;
    struct OBB;
    struct Sphere;
    struct Frustum;
    struct MiniMat;
    
    typedef glm::vec4 Color;
    typedef std::shared_ptr<class Texture> TexturePtr;
    typedef std::shared_ptr<class Material> MaterialPtr;
    typedef std::shared_ptr<class Geometry> GeometryPtr;
    typedef std::shared_ptr<class Object3D> Object3DPtr;
    typedef std::shared_ptr<class Mesh> MeshPtr;
    typedef std::shared_ptr<class Light> LightPtr;
    typedef std::weak_ptr<const Mesh> MeshWeakPtr;
    typedef std::shared_ptr<class Scene> ScenePtr;
    typedef std::shared_ptr<class Camera> CameraPtr;
    typedef std::shared_ptr<struct Bone> BonePtr;
    typedef std::shared_ptr<struct Animation> AnimationPtr;
    typedef std::shared_ptr<struct RenderBin> RenderBinPtr;
    
    enum Matrixtype { MODEL_VIEW_MATRIX = 1 << 0, PROJECTION_MATRIX = 1 << 1};
    KINSKI_API void pushMatrix(const Matrixtype type);
    KINSKI_API void popMatrix(const Matrixtype type);
    KINSKI_API void multMatrix(const Matrixtype type, const glm::mat4 &theMatrix);
    KINSKI_API void loadMatrix(const Matrixtype type, const glm::mat4 &theMatrix);
    KINSKI_API void getMatrix(const Matrixtype type, glm::mat4 &theMatrix);
    
    KINSKI_API void setMatrices( const CameraPtr &cam );
    KINSKI_API void setModelView( const CameraPtr &cam );
    KINSKI_API void setProjection( const CameraPtr &cam );
    KINSKI_API void setMatricesForWindow();
    
    const glm::vec3 X_AXIS = glm::vec3(1, 0, 0);
    const glm::vec3 Y_AXIS = glm::vec3(0, 1, 0);
    const glm::vec3 Z_AXIS = glm::vec3(0, 0, 1);
    
    static const Color COLOR_WHITE(1), COLOR_BLACK(0, 0, 0, 1), COLOR_RED(1, 0,  0, 1),
    COLOR_GREEN(0, 1, 0, 1), COLOR_BLUE(0, 0, 1, 1), COLOR_YELLOW(1, 1, 0, 1),
    COLOR_PURPLE(1, 0, 1, 1), COLOR_ORANGE(1, .5 , 0, 1), COLOR_OLIVE(.5, .5, 0, 1),
    COLOR_DARK_RED(.6, 0,  0, 1);
    
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

    KINSKI_API const glm::vec2& windowDimension();
    KINSKI_API void setWindowDimension(const glm::vec2 &theDim);
    KINSKI_API gl::Ray calculateRay(const CameraPtr &theCamera, uint32_t x, uint32_t y);
    KINSKI_API gl::AABB calculateAABB(const std::vector<glm::vec3> &theVertices);
    KINSKI_API glm::vec3 calculateCentroid(const std::vector<glm::vec3> &theVertices);
    KINSKI_API gl::MeshPtr createFrustumMesh(const CameraPtr &cam);
    
    /********************************* Drawing Functions *****************************************/
    
    KINSKI_API void clearColor(const Color &theColor);
    KINSKI_API void drawMesh(const MeshPtr &theMesh);
    KINSKI_API void drawLine(const glm::vec2 &a, const glm::vec2 &b,
                             const Color &theColor = Color(1),
                             float line_thickness = 1.f);
    KINSKI_API void drawLines2D(const std::vector<glm::vec3> &thePoints, const glm::vec4 &theColor,
                                float line_thickness = 1.f);
    KINSKI_API void drawLines(const std::vector<glm::vec3> &thePoints, const Color &theColor,
                              float line_thickness = 1.f);
    KINSKI_API void drawLines(const std::vector<glm::vec3> &thePoints, const MaterialPtr &theMat,
                              float line_thickness = 1.f);
    KINSKI_API void drawLineStrip(const std::vector<glm::vec3> &thePoints,
                                  const glm::vec4 &theColor,
                                  float line_thickness = 1.f);
    KINSKI_API void drawPoints(const gl::Buffer &the_point_buf,
                               const MaterialPtr &theMaterial = MaterialPtr(),
                               GLsizei offset = 0);
    KINSKI_API void drawPoints(const std::vector<glm::vec3> &thePoints,
                               const MaterialPtr &theMaterial = MaterialPtr());
    KINSKI_API void drawTexture(const gl::Texture &theTexture, const glm::vec2 &theSize,
                                const glm::vec2 &theTopLeft = glm::vec2(0));
    KINSKI_API void drawQuad(const MaterialPtr &theMaterial, const glm::vec2 &theSize,
                             const glm::vec2 &theTopLeft = glm::vec2(0), bool filled = true);
    KINSKI_API void drawQuad(const Color &theColor, const glm::vec2 &theSize,
                             const glm::vec2 &theTopLeft = glm::vec2(0), bool filled = true);
    KINSKI_API void drawQuad(const MaterialPtr &theMaterial,
                             float x0, float y0, float x1, float y1, bool filled = true);
    KINSKI_API void drawText2D(const std::string &theText, const gl::Font &theFont,
                               const Color &the_color = glm::vec4(1),
                               const glm::vec2 &theTopLeft = glm::vec2(0));
    KINSKI_API void drawText3D(const std::string &theText, const gl::Font &theFont);
    
    KINSKI_API void drawGrid(float width, float height, int numW = 20, int numH = 20);
    KINSKI_API void drawAxes(const MeshWeakPtr &theMesh);
    KINSKI_API void drawBoundingBox(const MeshWeakPtr &theMesh);
    KINSKI_API void drawNormals(const MeshWeakPtr &theMesh);
    KINSKI_API void drawCircle(const glm::vec2 &center, float radius, bool solid = true,
                               const MaterialPtr &theMaterial = MaterialPtr(),
                               int numSegments = 32 );
    
    KINSKI_API gl::Texture render_to_texture(const gl::Scene &theScene, gl::Fbo &theFbo,
                                             const gl::CameraPtr &theCam);
    
#ifdef KINSKI_CPP11
    KINSKI_API gl::Texture render_to_texture(gl::Fbo &theFbo, std::function<void()> functor);
#endif

    /*********************************** lazy state changing **********************************/
    
    KINSKI_API void apply_material(const MaterialPtr &the_mat, bool force_apply = false);
    
    /*********************************** inbuilt Texture loading **********************************/
    
    KINSKI_API MiniMat loadImage(const std::string &theFileName, int num_channels = 0);
    KINSKI_API Texture createTextureFromFile(const std::string &theFileName,
                                             bool mipmap = false,
                                             bool compress = false,
                                             GLfloat anisotropic_filter_lvl = 1.f);
    
    class ImageLoadException : public Exception
    {
    public:
        ImageLoadException(const std::string &thePath)
        :Exception("Got trouble with image file: " + thePath){};
    };
    
    /*********************************** Shader loading *******************************************/
    
    enum ShaderType {SHADER_UNLIT, SHADER_GOURAUD, SHADER_PHONG, SHADER_PHONG_NORMALMAP,
        SHADER_PHONG_SKIN, SHADER_POINTS_TEXTURE, SHADER_LINES, SHADER_POINTS_COLOR,
        SHADER_POINTS_SPHERE};
    KINSKI_API Shader createShader(ShaderType type);
    KINSKI_API Shader createShaderFromFile(const std::string &vertPath, const std::string &fragPath,
                                           const std::string &geomPath="");
    
    KINSKI_API const std::set<std::string>& getExtensions();
    KINSKI_API bool isExtensionSupported(const std::string &theName);
    
    //! Convenience class which pushes and pops the current viewport dimension
    class SaveViewPort
    {
     public:
        SaveViewPort(){m_old_value = windowDimension();}
        ~SaveViewPort(){setWindowDimension(m_old_value);}
     private:
        glm::vec2 m_old_value;
    };
    
    //! Convenience class which pushes and pops the currently bound framebuffer
    class SaveFramebufferBinding
    {
     public:
        SaveFramebufferBinding();
        ~SaveFramebufferBinding();
     private:
        GLint m_old_value;
    };
    
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
    
}}//namespace

#endif //_KINSKIGL_H
