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
#define KINSKI_GL_REPORT_ERRORS 0

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
#include <GL3/gl3.h>
#include <GL3/gl3ext.h>
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
#include "glm/gtx/norm.hpp"
#include "glm/gtc/noise.hpp"
#include "glm/gtc/random.hpp"
#include "glm/gtx/string_cast.hpp"

namespace kinski { namespace gl {

    // forward declarations
    class Buffer;
    class Shader;
    class Fbo;
    class Font;
    struct Ray;
    struct AABB;
    struct OBB;
    struct Sphere;
    struct Frustum;
    
    typedef glm::vec4 Color;
    typedef std::shared_ptr<class Texture> TexturePtr;
    typedef std::shared_ptr<class Material> MaterialPtr;
    typedef std::shared_ptr<class Geometry> GeometryPtr;
    typedef std::shared_ptr<class Object3D> Object3DPtr;
    typedef std::shared_ptr<class Mesh> MeshPtr;
    typedef std::weak_ptr<const Mesh> MeshWeakPtr;
    typedef std::shared_ptr<class Camera> CameraPtr;
    typedef std::shared_ptr<struct Bone> BonePtr;
    typedef std::shared_ptr<struct Animation> AnimationPtr;
    typedef std::shared_ptr<struct RenderBin> RenderBinPtr;
    
    enum Matrixtype { MODEL_VIEW_MATRIX = 1 << 0, PROJECTION_MATRIX = 1 << 1};
    KINSKI_API void pushMatrix(const Matrixtype type);
    KINSKI_API void popMatrix(const Matrixtype type);
    KINSKI_API void multMatrix(const glm::mat4 &theMatrix);
    KINSKI_API void loadMatrix(const Matrixtype type, const glm::mat4 &theMatrix);
    KINSKI_API void getMatrix(const Matrixtype type, glm::mat4 &theMatrix);
    
    KINSKI_API void setMatrices( const CameraPtr &cam );
    KINSKI_API void setModelView( const CameraPtr &cam );
    KINSKI_API void setProjection( const CameraPtr &cam );
    
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
    KINSKI_API gl::MeshPtr createFrustumMesh(const CameraPtr &cam);
    
    /********************************* Drawing Functions *****************************************/
    
    KINSKI_API void clearColor(const Color &theColor);
    KINSKI_API void drawLine(const glm::vec2 &a, const glm::vec2 &b, const Color &theColor = Color(1));
    KINSKI_API void drawLines(const std::vector<glm::vec3> &thePoints, const Color &theColor);
    KINSKI_API void drawPoints(GLuint thePointVBO, GLsizei theCount,
                               const MaterialPtr &theMaterial = std::shared_ptr<Material>(),
                               GLsizei stride = 0,
                               GLsizei offset = 0);
    KINSKI_API void drawPoints(const std::vector<glm::vec3> &thePoints,
                               const MaterialPtr &theMaterial = std::shared_ptr<Material>());
    KINSKI_API void drawTexture(const gl::Texture &theTexture, const glm::vec2 &theSize,
                                const glm::vec2 &theTopLeft = glm::vec2(0));
    KINSKI_API void drawQuad(const MaterialPtr &theMaterial, const glm::vec2 &theSize,
                             const glm::vec2 &theTopLeft = glm::vec2(0));
    KINSKI_API void drawQuad(const MaterialPtr &theMaterial,
                             float x0, float y0, float x1, float y1);
    KINSKI_API void drawText2D(const std::string &theText, const gl::Font &theFont,
                               const Color &the_color = glm::vec4(1),
                               const glm::vec2 &theTopLeft = glm::vec2(0));
    KINSKI_API void drawText3D(const std::string &theText, const gl::Font &theFont);
    KINSKI_API void drawMesh(const MeshPtr &theMesh);
    KINSKI_API void drawGrid(float width, float height, int numW = 20, int numH = 20);
    KINSKI_API void drawAxes(const MeshWeakPtr &theMesh);
    KINSKI_API void drawBoundingBox(const MeshWeakPtr &theMesh);
    KINSKI_API void drawNormals(const MeshWeakPtr &theMesh);
    KINSKI_API void drawSolidCircle(const glm::vec2 &center, float radius,
                                    const MaterialPtr &theMaterial = MaterialPtr(),
                                    int numSegments = 0 );

    /*********************************** lazy state changing **********************************/
    
    KINSKI_API void apply_material(const MaterialPtr &the_mat, bool force_apply = false);
    
    /*********************************** inbuilt Texture loading **********************************/
    
    KINSKI_API Texture createTextureFromFile(const std::string &theFileName, bool mipmap = false,
                                             bool compress = false, GLfloat anisotropic_filter_lvl = 1.f);
    
    class ImageLoadException : public Exception
    {
    public:
        ImageLoadException(const std::string &thePath)
        :Exception("Got trouble with image file: " + thePath){};
    };
    
    /*********************************** Shader loading *******************************************/
    
    enum ShaderType {SHADER_UNLIT, SHADER_PHONG, SHADER_PHONG_NORMALMAP, SHADER_PHONG_SKIN,
        SHADER_POINTS_TEXTURE, SHADER_POINTS_COLOR};
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

    #if KINSKI_GL_REPORT_ERRORS
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
