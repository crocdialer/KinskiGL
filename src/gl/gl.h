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

#include "core/core.h"
#include "core/file_functions.h"

//triggers checks with glGetError()
//#define KINSKI_GL_REPORT_ERRORS

#if defined(KINSKI_COCOA_TOUCH) || defined(KINSKI_RASPI)
#define KINSKI_GLES
#endif

#if defined(KINSKI_RASPI)
#define KINSKI_NO_VAO
#define GL_GLEXT_PROTOTYPES
#endif

#ifdef KINSKI_GLES // OpenGL ES

#ifdef KINSKI_COCOA_TOUCH // iOS
#ifdef KINSKI_GLES_3 // ES 3
#import <OpenGLES/ES3/gl.h>
#import <OpenGLES/ES3/glext.h>
#else // ES 2
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#endif // KINSKI_COCOA_TOUCH (iOS)

#else // general ES2
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif // KINSKI_GLES (OpenGL ES)

#elif defined(KINSKI_COCOA)// desktop GL3
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/glcorearb.h>
//TODO: remove this extension definitions from here, when a better place is found
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3
#endif

// crossplattform helper-macros to append either nothing or "OES"
// appropriately to a symbol based on either OpenGL3+ or OpenGLES2
#if defined(KINSKI_GLES) && !defined(KINSKI_GLES_3) // all versions, except ES2
#define GL_SUFFIX(sym) sym##OES
#define GL_ENUM(sym) sym##_OES
#else // only ES2 here
#define GL_SUFFIX(sym) sym
#define GL_ENUM(sym) sym
#endif

#define GLM_FORCE_CXX11
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

    // promote some glm symbols
    using glm::vec2;
    using glm::vec3;
    using glm::vec4;
    using glm::ivec2;
    using glm::ivec3;
    using glm::ivec4;
    using glm::mat2;
    using glm::mat3;
    using glm::mat4;
    using glm::quat;
    using glm::normalize;
    using glm::dot;
    using glm::cross;
    using glm::length;
    using glm::length2;
    using glm::translate;
    using glm::rotate;
    using glm::scale;
    
    // forward declarations
    class Buffer;
    class Shader;
    class Fbo;
    class Image;
    class Texture;
    class Font;
    class Visitor;
    struct Ray;
    struct AABB;
    struct OBB;
    struct Sphere;
    struct Frustum;
    
    typedef vec4 Color;
    typedef std::shared_ptr<class Material> MaterialPtr;
    typedef std::shared_ptr<class Geometry> GeometryPtr;
    typedef std::shared_ptr<class Object3D> Object3DPtr;
    typedef std::shared_ptr<class Mesh> MeshPtr;
    typedef std::weak_ptr<Mesh> MeshWeakPtr;
    typedef std::shared_ptr<struct MeshAnimation> MeshAnimationPtr;
    typedef std::shared_ptr<class Light> LightPtr;
    typedef std::shared_ptr<class Scene> ScenePtr;
    typedef std::shared_ptr<class Camera> CameraPtr;
    typedef std::shared_ptr<struct Bone> BonePtr;
    typedef std::shared_ptr<struct RenderBin> RenderBinPtr;
    
    enum Matrixtype { MODEL_VIEW_MATRIX = 1 << 0, PROJECTION_MATRIX = 1 << 1};
    KINSKI_API void pushMatrix(const Matrixtype type);
    KINSKI_API void popMatrix(const Matrixtype type);
    KINSKI_API void multMatrix(const Matrixtype type, const mat4 &theMatrix);
    KINSKI_API void loadMatrix(const Matrixtype type, const mat4 &theMatrix);
    KINSKI_API void loadIdentity(const Matrixtype type);
    KINSKI_API void getMatrix(const Matrixtype type, mat4 &theMatrix);
    
    KINSKI_API void setMatrices( const CameraPtr &cam );
    KINSKI_API void setModelView( const CameraPtr &cam );
    KINSKI_API void setProjection( const CameraPtr &cam );
    KINSKI_API void setMatricesForWindow();
    
    const vec3 X_AXIS = vec3(1, 0, 0);
    const vec3 Y_AXIS = vec3(0, 1, 0);
    const vec3 Z_AXIS = vec3(0, 0, 1);
    
    static const Color COLOR_WHITE(1), COLOR_BLACK(0, 0, 0, 1), COLOR_GRAY(.6, .6, .6, 1.),
    COLOR_RED(1, 0,  0, 1), COLOR_GREEN(0, 1, 0, 1), COLOR_BLUE(0, 0, 1, 1),
    COLOR_YELLOW(1, 1, 0, 1), COLOR_PURPLE(1, 0, 1, 1), COLOR_ORANGE(1, .5 , 0, 1),
    COLOR_OLIVE(.5, .5, 0, 1), COLOR_DARK_RED(.6, 0,  0, 1);
    
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

    KINSKI_API const vec2& windowDimension();
    KINSKI_API void setWindowDimension(const vec2 &theDim, const vec2 &the_offset = vec2(0));
    KINSKI_API gl::Ray calculateRay(const CameraPtr &theCamera, const vec2 &window_pos,
                                    const vec2 &window_size = windowDimension());
    KINSKI_API gl::AABB calculateAABB(const std::vector<vec3> &theVertices);
    KINSKI_API vec3 calculateCentroid(const std::vector<vec3> &theVertices);
    KINSKI_API gl::MeshPtr create_frustum_mesh(const CameraPtr &cam);
    KINSKI_API gl::CameraPtr create_shadow_camera(const LightPtr &the_light, float far_clip = 1000.f);
    KINSKI_API gl::CameraPtr create_shadow_camera(const Light *the_light, float far_clip = 1000.f);
    
    /*!
     * project a 3D point (in world coords) onto the view plane, using the provided camera object.
     * returns the 2D screen-coordinates of the projected 3D point
     */
    KINSKI_API vec2 project_point_to_screen(const vec3 &the_point,
                                            const CameraPtr &theCamera,
                                            const vec2 &screen_size = windowDimension());
    
    /********************************* Drawing Functions *****************************************/
    
    KINSKI_API void clear();
    KINSKI_API void clearColor(const Color &theColor);
    
    
    KINSKI_API void drawMesh(const MeshPtr &theMesh);
    KINSKI_API void drawLight(const LightPtr &theLight);
    KINSKI_API void drawLine(const vec2 &a, const vec2 &b,
                             const Color &theColor = Color(1),
                             float line_thickness = 1.f);
    KINSKI_API void drawLines2D(const std::vector<vec3> &thePoints, const vec4 &theColor,
                                float line_thickness = 1.f);
    KINSKI_API void drawLines(const std::vector<vec3> &thePoints, const Color &theColor,
                              float line_thickness = 1.f);
    KINSKI_API void drawLines(const std::vector<vec3> &thePoints, const MaterialPtr &theMat,
                              float line_thickness = 1.f);
    KINSKI_API void drawLineStrip(const std::vector<vec3> &thePoints,
                                  const vec4 &theColor,
                                  float line_thickness = 1.f);
    KINSKI_API void drawPoints(const gl::Buffer &the_point_buf,
                               const MaterialPtr &theMaterial = MaterialPtr(),
                               GLsizei offset = 0);
    KINSKI_API void drawPoints(const std::vector<vec3> &thePoints,
                               const MaterialPtr &theMaterial = MaterialPtr());
    KINSKI_API void drawTexture(const gl::Texture &theTexture, const vec2 &theSize,
                                const vec2 &theTopLeft = vec2(0));
    KINSKI_API void drawQuad(const MaterialPtr &theMaterial, const vec2 &theSize,
                             const vec2 &theTopLeft = vec2(0), bool filled = true);
    KINSKI_API void drawQuad(const Color &theColor, const vec2 &theSize,
                             const vec2 &theTopLeft = vec2(0), bool filled = true);
    KINSKI_API void drawQuad(const MaterialPtr &theMaterial,
                             float x0, float y0, float x1, float y1, bool filled = true);
    KINSKI_API void drawText2D(const std::string &theText, const gl::Font &theFont,
                               const Color &the_color = vec4(1),
                               const vec2 &theTopLeft = vec2(0));
    KINSKI_API void drawText3D(const std::string &theText, const gl::Font &theFont);
    
    KINSKI_API void drawGrid(float width, float height, int numW = 20, int numH = 20);
    KINSKI_API void drawAxes(const MeshWeakPtr &theMesh);
    KINSKI_API void drawTransform(const mat4& the_transform, float the_scale = 1.f);
    KINSKI_API void drawBoundingBox(const Object3DPtr &the_obj);
    KINSKI_API void drawNormals(const MeshWeakPtr &theMesh);
    KINSKI_API void drawCircle(const vec2 &center, float radius, bool solid = true,
                               int numSegments = 32, const MaterialPtr &theMaterial = MaterialPtr());
    
    KINSKI_API gl::Texture render_to_texture(const gl::Scene &theScene, gl::Fbo &theFbo,
                                             const gl::CameraPtr &theCam);
    
    KINSKI_API gl::Texture render_to_texture(gl::Fbo &theFbo, std::function<void()> functor);

    /*********************************** lazy state changing **********************************/
    
    KINSKI_API void apply_material(const MaterialPtr &the_mat, bool force_apply = false);
    
    /*********************************** inbuilt Texture loading **********************************/
    
    KINSKI_API Image decode_image(const std::vector<uint8_t> &the_data, int num_channels = 0);
    KINSKI_API Texture createTextureFromFile(const std::string &theFileName,
                                             bool mipmap = false,
                                             bool compress = false,
                                             GLfloat anisotropic_filter_lvl = 1.f);
    KINSKI_API Texture createTextureFromData(const std::vector<uint8_t> &the_data,
                                             bool mipmap = false,
                                             bool compress = false,
                                             GLfloat anisotropic_filter_lvl = 1.f);
    
    /*!
     * create a gl::Texture object of type GL_TEXTURE_CUBE 
     * from 6 individual gl::Texture objects of type GL_TEXTURE_2D of same size
     */
    KINSKI_API Texture create_cube_texture(const std::vector<gl::Texture> &the_planes);
    
    class ImageLoadException : public Exception
    {
    public:
        ImageLoadException()
        :Exception("Got trouble decoding image file"){};
    };
    
    /*********************************** Shader loading *******************************************/
    
    enum class ShaderType {UNLIT, UNLIT_SKIN, BLUR, GOURAUD, PHONG, PHONG_SHADOWS, PHONG_SKIN_SHADOWS,
        PHONG_NORMALMAP, PHONG_SKIN, POINTS_TEXTURE, LINES_2D, POINTS_COLOR, POINTS_SPHERE, RECT_2D,
        NOISE_3D};
    
    KINSKI_API Shader createShader(ShaderType type, bool use_cached_shader = true);
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
        vec2 m_old_value;
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
    
    template<typename T> class scoped_bind
    {
    public:
        scoped_bind():
        m_obj(nullptr),m_isBound(false)
        {}
        
        explicit scoped_bind(const T &theObj):
        m_obj(&theObj),m_isBound(false)
        {
            bind();
        }
        
        ~scoped_bind()
        {
            if(m_isBound)
                unbind();
        }
        
        inline void bind()
        {
            m_obj->bind();
            m_isBound = true;
        }
        
        inline void unbind()
        {
            m_obj->unbind();
            m_isBound = false;
        }
        
    private:
        const T *m_obj;
        bool m_isBound;
    };
    
    /*!
     * unfinished geometry check
     * return true if point p is contained within the mesh's geometry 
     * TODO: only convex or concave?
     */
    KINSKI_API bool is_point_inside_mesh(const vec3& p, gl::MeshPtr m);
    
    /*!
     * generate texcoords for dest mesh by raycasting the src mesh
     */
    KINSKI_API void project_texcoords(gl::MeshPtr src, gl::MeshPtr dest);
    
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