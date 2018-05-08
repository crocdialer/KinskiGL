// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "core/core.hpp"
#include "core/Image.hpp"
#include "Platform.h"


#define GLM_FORCE_CXX11
#define GLM_FORCE_SWIZZLE
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/noise.hpp"
#include "glm/gtc/random.hpp"
#include "glm/gtx/norm.hpp"
#include "glm/gtx/string_cast.hpp"

#include "SerializerGL.hpp"

namespace kinski { namespace gl {

// promote some glm symbols
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::dvec2;
using glm::dvec3;
using glm::dvec4;
using glm::ivec2;
using glm::ivec3;
using glm::ivec4;
using glm::uvec2;
using glm::uvec3;
using glm::uvec4;
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

template<uint8_t DIM, typename T> struct KINSKI_API Vector{};

template<> struct KINSKI_API Vector<2, float>{ using Type = vec2; };
template<> struct KINSKI_API Vector<3, float>{ using Type = vec3; };
template<> struct KINSKI_API Vector<4, float>{ using Type = vec4; };
template<> struct KINSKI_API Vector<2, double>{ using Type = dvec2; };
template<> struct KINSKI_API Vector<3, double>{ using Type = dvec3; };
template<> struct KINSKI_API Vector<4, double>{ using Type = dvec4; };
template<> struct KINSKI_API Vector<2, int32_t>{ using Type = ivec2; };
template<> struct KINSKI_API Vector<3, int32_t>{ using Type = ivec3; };
template<> struct KINSKI_API Vector<4, int32_t>{ using Type = ivec4; };
template<> struct KINSKI_API Vector<2, uint32_t>{ using Type = ivec2; };
template<> struct KINSKI_API Vector<3, uint32_t>{ using Type = ivec3; };
template<> struct KINSKI_API Vector<4, uint32_t>{ using Type = ivec4; };
    
// forward declarations
class Buffer;
class Texture;
class Font;
class Visitor;
struct Ray;
struct AABB;
struct OBB;
struct Sphere;
struct Frustum;

using Color = vec4;
DEFINE_CLASS_PTR(Fbo);
DEFINE_CLASS_PTR(Shader);
DEFINE_CLASS_PTR(Material);
DEFINE_CLASS_PTR(Geometry);
DEFINE_CLASS_PTR(Object3D);
DEFINE_CLASS_PTR(Mesh);
DEFINE_CLASS_PTR(MeshAnimation);
DEFINE_CLASS_PTR(Light);
DEFINE_CLASS_PTR(Camera);
DEFINE_CLASS_PTR(Bone);
DEFINE_CLASS_PTR(Scene);

class Context
{
public:

    Context(std::shared_ptr<PlatformData> platform_data);
    std::shared_ptr<PlatformData> platform_data();
    void* current_context_id();
    void set_current_context_id(void* the_id);

    uint32_t get_vao(const gl::GeometryPtr &the_geom, const gl::ShaderPtr &the_shader);
    uint32_t create_vao(const gl::GeometryPtr &the_geom, const gl::ShaderPtr &the_shader);

    uint32_t get_fbo(const gl::Fbo* the_fbo, uint32_t the_index);
    uint32_t create_fbo(const gl::Fbo* the_fbo, uint32_t the_index);
    void clear_assets_for_context(void* the_context_id);

private:
    std::unique_ptr<struct ContextImpl> m_impl;
};

KINSKI_API void create_context(const std::shared_ptr<PlatformData> &the_platform_data);
KINSKI_API const std::unique_ptr<Context>& context();

enum Matrixtype { MODEL_VIEW_MATRIX = 1 << 0, PROJECTION_MATRIX = 1 << 1};
KINSKI_API void push_matrix(const Matrixtype type);
KINSKI_API void pop_matrix(const Matrixtype type);
KINSKI_API void mult_matrix(const Matrixtype type, const mat4 &theMatrix);
KINSKI_API void load_matrix(const Matrixtype type, const mat4 &theMatrix);
KINSKI_API void load_identity(const Matrixtype type);
KINSKI_API void get_matrix(const Matrixtype type, mat4 &theMatrix);

KINSKI_API void set_matrices(const CameraPtr &cam);
KINSKI_API void set_modelview(const CameraPtr &cam);
KINSKI_API void set_projection(const CameraPtr &cam);
KINSKI_API void set_matrices_for_window();

static const vec3 X_AXIS = vec3(1, 0, 0);
static const vec3 Y_AXIS = vec3(0, 1, 0);
static const vec3 Z_AXIS = vec3(0, 0, 1);

static const Color COLOR_WHITE(1), COLOR_BLACK(0, 0, 0, 1), COLOR_GRAY(.6, .6, .6, 1.),
COLOR_RED(1, 0,  0, 1), COLOR_GREEN(0, 1, 0, 1), COLOR_BLUE(0, 0, 1, 1),
COLOR_YELLOW(1, 1, 0, 1), COLOR_PURPLE(1, 0, 1, 1), COLOR_ORANGE(1, .5 , 0, 1),
COLOR_OLIVE(.5, .5, 0, 1), COLOR_DARK_RED(.6, 0,  0, 1);

class ScopedMatrixPush
{
public:
    ScopedMatrixPush(const Matrixtype type):
    m_type(type)
    {push_matrix(type);}

    ~ScopedMatrixPush()
    {pop_matrix(m_type);}
private:
    Matrixtype m_type;
};

//! helper macro for buffer offsets
#define BUFFER_OFFSET(i) ((char *)nullptr + (i))
    
KINSKI_API const vec2& window_dimension();
KINSKI_API void set_window_dimension(const vec2 &theDim, const vec2 &the_offset = vec2(0));
KINSKI_API float aspect_ratio();
KINSKI_API gl::Ray calculate_ray(const CameraPtr &theCamera, const vec2 &window_pos,
                                 const vec2 &window_size = window_dimension());
KINSKI_API gl::MeshPtr create_frustum_mesh(const CameraPtr &cam, bool solid = false);
KINSKI_API gl::CameraPtr create_shadow_camera(const LightPtr &the_light, float far_clip = 1000.f);
KINSKI_API gl::CameraPtr create_shadow_camera(const Light *the_light, float far_clip = 1000.f);

/*!
 * project a 3D point (in world coords) onto the view plane, using the provided camera object.
 * returns the 2D screen-coordinates of the projected 3D point
 */
KINSKI_API vec2 project_point_to_screen(const vec3 &the_point,
                                        const CameraPtr &theCamera,
                                        const vec2 &screen_size = window_dimension());

/********************************* Drawing Functions *****************************************/

KINSKI_API void clear();
KINSKI_API void clear_color(const Color &theColor);
    
KINSKI_API void clear(const gl::Color &the_color);


KINSKI_API void draw_mesh(const MeshPtr &theMesh, const ShaderPtr &overide_shader = ShaderPtr());
KINSKI_API void draw_light(const LightPtr &theLight);
KINSKI_API void draw_line(const vec2 &a, const vec2 &b,
                          const Color &theColor = COLOR_WHITE,
                          float line_thickness = 1.f);
KINSKI_API void draw_lines_2D(const std::vector<vec3> &thePoints, const vec4 &theColor,
                              float line_thickness = 1.f);
KINSKI_API void draw_lines(const std::vector<vec3> &thePoints, const Color &theColor,
                           float line_thickness = 1.f);
KINSKI_API void draw_lines(const std::vector<vec3> &thePoints, const MaterialPtr &theMat,
                           float line_thickness = 1.f);
KINSKI_API void draw_linestrip(const std::vector<vec3> &thePoints,
                               const vec4 &theColor,
                               float line_thickness = 1.f);
KINSKI_API void draw_points_2D(const std::vector<vec2> &the_points, gl::Color the_color,
                               float the_point_size = 1.f);
KINSKI_API void draw_points(const std::vector<vec3> &the_points, const MaterialPtr &theMaterial,
                            float the_point_size = 1.f);
KINSKI_API void draw_texture(const gl::Texture &theTexture, const vec2 &theSize,
                             const vec2 &theTopLeft = vec2(0), const float the_brightness = 1.f);
KINSKI_API void draw_texture_with_mask(const gl::Texture &the_texture,
                                       const gl::Texture &the_mask,
                                       const vec2 &theSize,
                                       const vec2 &theTopLeft = vec2(0),
                                       const float the_brightness = 1.f);
KINSKI_API void draw_quad(const vec2 &the_size, const MaterialPtr &the_material,
                          const vec2 &the_topleft = vec2(0), bool fill = true);
KINSKI_API void draw_quad(const vec2 &the_size, const Color &the_color,
                          const vec2 &the_topleft = vec2(0), bool fill = true);
KINSKI_API void draw_quad(const MaterialPtr &theMaterial,
                          float x0, float y0, float x1, float y1, bool filled = true);
KINSKI_API void draw_text_2D(const std::string &theText, const gl::Font &theFont,
                             const Color &the_color = gl::COLOR_WHITE,
                             const vec2 &theTopLeft = vec2(0));

KINSKI_API void draw_grid(float width, float height, int numW = 20, int numH = 20);
KINSKI_API void draw_transform(const mat4& the_transform, float the_scale = 1.f);
KINSKI_API void draw_boundingbox(const gl::AABB& the_aabb);
KINSKI_API void draw_normals(const MeshWeakPtr &theMesh);
KINSKI_API void draw_circle(const vec2 &center, float radius, const gl::Color &the_color,
                            bool solid = true, uint32_t the_num_segments = 0);
KINSKI_API void draw_circle(const vec2 &center, float radius, const MaterialPtr &theMaterial,
                            bool solid = true, uint32_t the_num_segments = 0);

KINSKI_API gl::Texture render_to_texture(const gl::SceneConstPtr &theScene, const FboPtr &the_fbo,
                                         const gl::CameraPtr &theCam);

KINSKI_API gl::Texture render_to_texture(const FboPtr &the_fbo, std::function<void()> the_functor);

/*********************************** lazy state changing **********************************/

KINSKI_API void apply_material(const MaterialPtr &the_mat, bool force_apply = false,
                               const ShaderPtr &override_shader = gl::ShaderPtr());

/*!
 * resets the OpenGL state to default values
 */
KINSKI_API void reset_state();

/*********************************** Shader loading *******************************************/

enum class ShaderType {NONE, UNLIT, UNLIT_MASK, UNLIT_SKIN, UNLIT_CUBE, UNLIT_PANORAMA, RESOLVE, BLUR,
    GOURAUD, GOURAUD_SKIN, PHONG, PHONG_SHADOWS,
    PHONG_SKIN_SHADOWS, PHONG_NORMALMAP, PHONG_SKIN, POINTS_TEXTURE, LINES_2D, POINTS_COLOR,
    POINTS_SPHERE, RECT_2D, NOISE_3D, DEPTH_OF_FIELD, SDF_FONT};

KINSKI_API ShaderPtr create_shader(ShaderType type, bool use_cached_shader = true);
KINSKI_API ShaderPtr create_shader_from_file(const std::string &vertPath, const std::string &fragPath,
                                             const std::string &geomPath="");

KINSKI_API const std::string& get_shader_name(ShaderType the_type);
KINSKI_API const std::set<std::string>& get_extensions();
KINSKI_API bool is_extension_supported(const std::string &theName);

/************************************* Texture loading ****************************************/

KINSKI_API void get_texture_format(int the_num_comps, bool compress, GLenum the_datatype, GLenum *out_format,
                                   GLenum *out_internal_format);
    
KINSKI_API Texture create_texture_from_file(const std::string &the_path,
                                            bool mipmap = false,
                                            bool compress = false,
                                            GLfloat anisotropic_filter_lvl = 1.f);

KINSKI_API Texture create_texture_from_image(const ImagePtr &the_img, bool mipmap = false,
                                             bool compress = false,
                                             GLfloat anisotropic_filter_lvl = 1.f);

KINSKI_API Texture create_texture_from_data(const std::vector<uint8_t> &the_data,
                                            bool mipmap = false,
                                            bool compress = false,
                                            GLfloat anisotropic_filter_lvl = 1.f);

KINSKI_API ImagePtr create_image_from_texture(const gl::Texture &the_texture);

enum class CubeTextureLayout{V_CROSS, H_CROSS, V_STRIP, H_STRIP};

KINSKI_API Texture create_cube_texture_from_file(const std::string &the_path,
                                                 CubeTextureLayout the_layout = CubeTextureLayout::H_CROSS,
                                                 bool compress = false);

KINSKI_API gl::Texture create_cube_texture_from_images(const std::vector<ImagePtr> &the_planes,
                                                       bool mipmap = false,
                                                       bool compress = false);

KINSKI_API gl::Texture create_cube_texture_from_panorama(const gl::Texture &the_panorama,
                                                         size_t the_size,
                                                         bool mipmap = false,
                                                         bool compress = false);

//! Convenience class which pushes and pops the current viewport dimension
class SaveViewPort
{
 public:
    SaveViewPort(){m_old_value = window_dimension();}
    ~SaveViewPort(){set_window_dimension(m_old_value);}
 private:
    vec2 m_old_value;
};

//! Convenience class which pushes and pops the currently bound framebuffer
class SaveFramebufferBinding
{
 public:
    SaveFramebufferBinding();
    ~SaveFramebufferBinding();
    int value(){ return m_old_value; }
 private:
    GLint m_old_value;
};

template<typename T> class scoped_bind
{
public:
    explicit scoped_bind(T *theObj): m_obj(theObj){ m_obj->bind(); }
    explicit scoped_bind(const std::shared_ptr<T> &theObj): scoped_bind(theObj.get()){}
    explicit scoped_bind(T &theObj): scoped_bind(&theObj){}
    ~scoped_bind(){ m_obj->unbind(); }

private:
    T *m_obj;
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
