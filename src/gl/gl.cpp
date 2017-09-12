// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "core/file_functions.hpp"
#include "gl.hpp"
#include "Material.hpp"
#include "Shader.hpp"
#include "ShaderLibrary.h"
#include "Texture.hpp"
#include "Camera.hpp"
#include "Mesh.hpp"
#include "Font.hpp"
#include "Scene.hpp"
#include "Fbo.hpp"

using namespace glm;
using namespace std;

// maximum matrix-stack size
#define MAX_MATRIX_STACK_SIZE 100

namespace kinski { namespace gl {
    
///////////////////////////////////////////////////////////////////////////////

    namespace
    {
        Context* g_context = nullptr;
        
        std::map<gl::ShaderType, std::string> g_shader_names =
        {
            {ShaderType::UNKNOWN, "UNKNOWN"},
            {ShaderType::UNLIT, "UNLIT"}, {ShaderType::UNLIT_MASK, "UNLIT_MASK"},
            {ShaderType::UNLIT_SKIN, "UNLIT_SKIN"}, {ShaderType::BLUR, "BLUR"},
            {ShaderType::GOURAUD, "GOURAUD"}, {ShaderType::PHONG, "PHONG"},
            {ShaderType::PHONG_SHADOWS, "PHONG_SHADOWS"},
            {ShaderType::PHONG_SKIN_SHADOWS, "PHONG_SKIN_SHADOWS"},
            {ShaderType::PHONG_NORMALMAP, "PHONG_NORMALMAP"},
            {ShaderType::PHONG_SKIN, "PHONG_SKIN"}, {ShaderType::POINTS_TEXTURE, "POINTS_TEXTURE"},
            {ShaderType::LINES_2D, "LINES_2D"}, {ShaderType::POINTS_COLOR, "POINTS_COLOR"},
            {ShaderType::POINTS_SPHERE, "POINTS_SPHERE"}, {ShaderType::RECT_2D, "RECT_2D"},
            {ShaderType::NOISE_3D, "NOISE_3D"}, {ShaderType::DEPTH_OF_FIELD, "DEPTH_OF_FIELD"},
            {ShaderType::SDF_FONT, "SDF_FONT"}
        };
    };

    struct ContextImpl
    {
        std::shared_ptr<Context::PlatformData> m_platform_data;
        void* m_current_context_id = nullptr;
        //std::map<std::pair<void*, gl::MeshWeakPtr>, std::vector<GLuint>> m_vao_map;
    };

    Context::Context(std::shared_ptr<PlatformData> platform_data):m_impl(new ContextImpl)
    {
        m_impl->m_platform_data = platform_data;
    }

    std::shared_ptr<Context::PlatformData> Context::platform_data()
    {
        return m_impl->m_platform_data;
    }

    void* Context::current_context_id()
    {
        return m_impl->m_current_context_id;
    }

    void Context::set_current_context_id(void* the_id)
    {
        m_impl->m_current_context_id = the_id;
    }


    void set_context(Context* the_ctx)
    {
        if(g_context){ delete(g_context); }
        g_context = the_ctx;
    }

    Context* context(){ return g_context; }

    static glm::vec2 g_viewport_dim;
    static std::stack<glm::mat4> g_projectionMatrixStack;
    static std::stack<glm::mat4> g_modelViewMatrixStack;
    static gl::MaterialPtr g_line_material;
    static std::map<ShaderType, ShaderPtr> g_shaders;

///////////////////////////////////////////////////////////////////////////////

    void push_matrix(const Matrixtype type)
    {
        if(g_modelViewMatrixStack.size() > MAX_MATRIX_STACK_SIZE ||
           g_projectionMatrixStack.size() > MAX_MATRIX_STACK_SIZE)
        {
            throw Exception("Matrix stack overflow");
        }

        switch (type)
        {
            case PROJECTION_MATRIX:
                g_projectionMatrixStack.push(g_projectionMatrixStack.top());
                break;
            case MODEL_VIEW_MATRIX:
                g_modelViewMatrixStack.push(g_modelViewMatrixStack.top());
                break;

            default:
                break;
        }
    }

///////////////////////////////////////////////////////////////////////////////

    void pop_matrix(const Matrixtype type)
    {
        switch (type)
        {
            case PROJECTION_MATRIX:
                if(g_projectionMatrixStack.size() > 1) g_projectionMatrixStack.pop();
                break;
            case MODEL_VIEW_MATRIX:
                if(g_modelViewMatrixStack.size() > 1) g_modelViewMatrixStack.pop();
                break;

            default:
                break;
        }
    }

///////////////////////////////////////////////////////////////////////////////

    void mult_matrix(const Matrixtype type, const glm::mat4 &theMatrix)
    {
        switch (type)
        {
            case PROJECTION_MATRIX:
                g_projectionMatrixStack.top() *= theMatrix;
                break;
            case MODEL_VIEW_MATRIX:
                g_modelViewMatrixStack.top() *= theMatrix;
                break;

            default:
                break;
        }
    }

///////////////////////////////////////////////////////////////////////////////

    void load_matrix(const Matrixtype type, const glm::mat4 &theMatrix)
    {
        switch (type)
        {
            case PROJECTION_MATRIX:
                g_projectionMatrixStack.top() = theMatrix;
                break;
            case MODEL_VIEW_MATRIX:
                g_modelViewMatrixStack.top() = theMatrix;
                break;

            default:
                break;
        }
    }

///////////////////////////////////////////////////////////////////////////////

    void load_identity(const Matrixtype type)
    {
        load_matrix(type, glm::mat4());
    }

///////////////////////////////////////////////////////////////////////////////

    void get_matrix(const Matrixtype type, glm::mat4 &theMatrix)
    {
        switch (type)
        {
            case PROJECTION_MATRIX:
                theMatrix = g_projectionMatrixStack.top();
                break;
            case MODEL_VIEW_MATRIX:
                theMatrix = g_modelViewMatrixStack.top();
                break;

            default:
                break;
        }
    }

///////////////////////////////////////////////////////////////////////////////

    void set_matrices(const CameraPtr &cam)
    {
        set_projection(cam);
        set_modelview(cam);
    }

///////////////////////////////////////////////////////////////////////////////

    void set_modelview(const CameraPtr &cam)
    {
        load_matrix(MODEL_VIEW_MATRIX, cam->view_matrix());
    }

///////////////////////////////////////////////////////////////////////////////

    void set_projection(const CameraPtr &cam)
    {
        load_matrix(PROJECTION_MATRIX, cam->projection_matrix());
    }

    ///////////////////////////////////////////////////////////////////////////////

    void set_matrices_for_window()
    {
        load_matrix(gl::PROJECTION_MATRIX, glm::ortho(0.f, gl::window_dimension().x,
                                                      0.f, gl::window_dimension().y,
                                                      0.f, 1.f));
        load_matrix(gl::MODEL_VIEW_MATRIX, mat4());
    }

///////////////////////////////////////////////////////////////////////////////

    const glm::vec2& window_dimension(){ return g_viewport_dim; }

///////////////////////////////////////////////////////////////////////////////

    void set_window_dimension(const glm::vec2 &theDim, const vec2 &the_offset)
    {
        g_viewport_dim = theDim;
        glViewport(the_offset.x, the_offset.y, theDim.x, theDim.y);

        if(g_projectionMatrixStack.empty())
            g_projectionMatrixStack.push(mat4());

        if(g_modelViewMatrixStack.empty())
            g_modelViewMatrixStack.push(mat4());
    }

///////////////////////////////////////////////////////////////////////////////

    float aspect_ratio()
    {
        return (g_viewport_dim.y) != 0 ? (g_viewport_dim.x / g_viewport_dim.y) : (16.f / 9.f);
    }

///////////////////////////////////////////////////////////////////////////////

    gl::Ray calculate_ray(const CameraPtr &theCamera, const glm::vec2 &window_pos,
                          const glm::vec2 &window_size)
    {
        glm::vec3 cam_pos = theCamera->position();
        glm::vec3 lookAt = theCamera->lookAt(),
        side = theCamera->side(), up = theCamera->up();
        float near = theCamera->near();
        glm::vec3 click_world_pos, ray_dir;

        if(PerspectiveCamera::Ptr cam = dynamic_pointer_cast<PerspectiveCamera>(theCamera))
        {
            // bring click_pos to range -1, 1
            glm::vec2 click_2D(window_pos);
            glm::vec2 offset (window_size / 2.0f);
            click_2D -= offset;
            click_2D /= offset;
            click_2D.y = - click_2D.y;

            // convert fovy to radians
            float rad = glm::radians(cam->fov());
            float vLength = tan( rad / 2) * near;
            float hLength = vLength * cam->aspect();

            click_world_pos = cam_pos + lookAt * near
                + side * hLength * click_2D.x
                + up * vLength * click_2D.y;
            ray_dir = click_world_pos - cam_pos;

        }else if (OrthographicCamera::Ptr cam = dynamic_pointer_cast<OrthographicCamera>(theCamera))
        {
            gl::vec2 coord(map_value<float>(window_pos.x, 0, window_size.x, cam->left(), cam->right()),
                           map_value<float>(window_pos.y, window_size.y, 0, cam->bottom(), cam->top()));
            click_world_pos = cam_pos + lookAt * near + side * coord.x + up  * coord.y;
            ray_dir = lookAt;
        }
        LOG_TRACE_2 << "clicked_world: (" << click_world_pos.x << ",  " << click_world_pos.y
            << ",  " << click_world_pos.z << ")";
        return Ray(click_world_pos, ray_dir);
    }

///////////////////////////////////////////////////////////////////////////////

    gl::MeshPtr create_frustum_mesh(const CameraPtr &cam, bool solid)
    {
        glm::mat4 inverse_projection = glm::inverse(cam->projection_matrix());
        gl::GeometryPtr geom = Geometry::create();

        const glm::vec3 vertices[8] = {vec3(-1, -1, -1), vec3(1, -1, -1), vec3(1, 1, -1), vec3(-1, 1, -1),
                                       vec3(-1, -1, 1), vec3(1, -1, 1), vec3(1, 1, 1), vec3(-1, 1, 1)};

        auto append_quad = [geom](uint32_t a, uint32_t b, uint32_t c, uint32_t d)
        {
            geom->append_face(a, b, c);
            geom->append_face(c, d, a);
        };

        for (int i = 0; i < 8; i++)
        {
            vec4 proj_v = inverse_projection * vec4(vertices[i], 1.f);
            geom->vertices().push_back(vec3(proj_v) / proj_v.w);
        }

        if(solid)
        {
            geom->set_primitive_type(GL_TRIANGLES);
            append_quad(0, 1, 2, 3);
            append_quad(4, 0, 3, 7);
            append_quad(1, 5, 6, 2);
            append_quad(3, 2, 6, 7);
            append_quad(4, 5, 1, 0);
            append_quad(5, 4, 7, 6);
        }
        else
        {
            geom->set_primitive_type(GL_LINE_STRIP);
            const index_t indices[] = {0, 1, 2, 3, 0, 4, 5, 6, 7, 4, 0, 3, 7, 6, 2, 1, 5};
            size_t num_indices = sizeof(indices) / sizeof(index_t);
            geom->append_indices(indices, num_indices);
        }

        geom->compute_bounding_box();
        gl::MaterialPtr mat = gl::Material::create();
        gl::MeshPtr m = gl::Mesh::create(geom, mat);
        m->set_transform(cam->transform());
        return m;
    }

///////////////////////////////////////////////////////////////////////////////

    gl::CameraPtr create_shadow_camera(const LightPtr &the_light, float far_clip)
    {
        return create_shadow_camera(the_light.get(), far_clip);
    }

    gl::CameraPtr create_shadow_camera(const Light *the_light, float far_clip)
    {
        gl::Camera::Ptr cam;

        switch(the_light->type())
        {
            case gl::Light::DIRECTIONAL:
            {
                float v = 512;
                cam = gl::OrthographicCamera::create(-v, v, -v, v, .1f, far_clip);
                break;
            }
            case gl::Light::POINT:
                cam = gl::CubeCamera::create(.1f, far_clip);
                break;
            case gl::Light::SPOT:
                cam = gl::PerspectiveCamera::create(1.f, 2 * the_light->spot_cutoff(),
                                                    .1f, far_clip);
                break;
        }
        cam->set_transform(the_light->global_transform());
        return cam;
    }

///////////////////////////////////////////////////////////////////////////////

    glm::vec2 project_point_to_screen(const glm::vec3 &the_point,
                                      const CameraPtr &theCamera,
                                      const glm::vec2 &screen_size)
    {
        glm::vec2 ret;

        if(theCamera)
        {
            // obtain normalized device coords (-1, 1)
            glm::vec4 p = vec4(the_point, 1.f);
            p = theCamera->projection_matrix() * theCamera->view_matrix() * p;

            // divide by w
            p /= p.w;

            // bring to range (0, 1)
            p += vec4(1);
            p /= 2.f;

            // flip to upper left coords
            p.y = 1.f - p.y;

            ret = vec2(p.xy()) * screen_size;
        }
        return ret;
    }

///////////////////////////////////////////////////////////////////////////////

    void clear_color(const Color &theColor)
    {
        glClearColor(theColor.r, theColor.g, theColor.b, theColor.a);
    }

///////////////////////////////////////////////////////////////////////////////

    void clear()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_line(const vec2 &a, const vec2 &b, const Color &theColor, float line_thickness)
    {
        static vector<vec3> thePoints;
        thePoints.clear();
        thePoints.push_back(vec3(a, 0));
        thePoints.push_back(vec3(b, 0));
        draw_lines_2D(thePoints, theColor, line_thickness);
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_lines_2D(const vector<vec3> &thePoints, const vec4 &theColor, float line_thickness)
    {
        ScopedMatrixPush pro(gl::PROJECTION_MATRIX), mod(gl::MODEL_VIEW_MATRIX);

        load_matrix(gl::PROJECTION_MATRIX, glm::ortho(0.f, g_viewport_dim[0],
                                                     0.f, g_viewport_dim[1],
                                                     0.f, 1.f));
        load_matrix(gl::MODEL_VIEW_MATRIX, mat4());
        draw_lines(thePoints, theColor, line_thickness);
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_lines(const vector<vec3> &thePoints, const Color &the_color,
                   float line_thickness)
    {
        static MaterialPtr material;
        if(!material)
        {
            material = gl::Material::create(gl::create_shader(gl::ShaderType::LINES_2D));
            material->set_depth_test(false);
        }
        material->set_diffuse(the_color);
        material->set_blending(the_color.a < 1.f);

        draw_lines(thePoints, material, line_thickness);
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_lines(const vector<vec3> &thePoints, const MaterialPtr &the_material,
                   float line_thickness)
    {
        if(thePoints.empty()) return;
        static MeshPtr mesh;
        static MaterialPtr material;

        //create line mesh
        if(!mesh)
        {
            material = gl::Material::create(gl::create_shader(gl::ShaderType::LINES_2D));
            material->set_blending();
            material->set_two_sided();
            gl::GeometryPtr geom = Geometry::create();
            mesh = gl::Mesh::create(geom, material);

            //mesh->geometry()->setPrimitiveType(GL_LINES_ADJACENCY);
            mesh->geometry()->set_primitive_type(GL_LINES);
        }

        mesh->material() = (the_material ? the_material : material);
        mesh->material()->uniform("u_window_size", window_dimension());
        mesh->material()->uniform("u_line_thickness", line_thickness);
        mesh->material()->set_line_width(line_thickness);

        mesh->geometry()->append_vertices(thePoints);
        mesh->geometry()->colors().resize(thePoints.size(), mesh->material()->diffuse());
        mesh->geometry()->tex_coords().resize(thePoints.size(), gl::vec2(0));
        mesh->geometry()->create_gl_buffers();
        gl::draw_mesh(mesh);
        mesh->geometry()->vertices().clear();
        mesh->geometry()->colors().clear();
        mesh->geometry()->indices().clear();
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_linestrip(const vector<vec3> &thePoints, const vec4 &theColor, float line_thickness)
    {
        if(thePoints.empty()) return;
        static gl::MeshPtr mesh;

        //create line mesh
        if(!mesh)
        {
            gl::MaterialPtr mat = gl::Material::create();
            mat->set_two_sided();
            gl::GeometryPtr geom = Geometry::create();
            mesh = gl::Mesh::create(geom, mat);
            mesh->geometry()->set_primitive_type(GL_LINE_STRIP);
        }
        mesh->material()->uniform("u_window_size", window_dimension());
        mesh->material()->uniform("u_line_thickness", line_thickness);
        mesh->geometry()->append_vertices(thePoints);
        mesh->geometry()->colors().resize(thePoints.size(), theColor);
        mesh->geometry()->create_gl_buffers();
        gl::draw_mesh(mesh);
        mesh->geometry()->vertices().clear();
        mesh->geometry()->colors().clear();
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_points_2D(const std::vector<vec2> &the_points, gl::Color the_color,
                        float the_point_size)
    {
        static gl::MaterialPtr material;

        //create material, if not yet here
        if(!material)
        {
            material = gl::Material::create();
            material->set_shader(gl::create_shader(gl::ShaderType::POINTS_COLOR));
            material->set_depth_test(false);
            material->set_depth_write(false);
            material->set_blending(true);
        }
        material->set_diffuse(the_color);
        ScopedMatrixPush pro(gl::PROJECTION_MATRIX), mod(gl::MODEL_VIEW_MATRIX);

        load_matrix(gl::PROJECTION_MATRIX, glm::ortho(0.f, g_viewport_dim[0],
                                                      0.f, g_viewport_dim[1],
                                                      0.f, 1.f));
        load_matrix(gl::MODEL_VIEW_MATRIX, mat4());

        // invert coords to 2D with TL center
        std::vector<gl::vec3> inverted_points;

        for(auto &p : the_points)
        {
            inverted_points.push_back(gl::vec3(p.x, g_viewport_dim[1] - p.y, 0.0f));
        }
        draw_points(inverted_points, material, the_point_size);
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_points(const std::vector<glm::vec3> &the_points, const MaterialPtr &the_material,
                     float the_point_size)
    {
        static MeshPtr point_mesh;

        if(!the_material){ return; }
        if(!point_mesh)
        {
            point_mesh = gl::Mesh::create();
            point_mesh->geometry()->set_primitive_type(GL_POINTS);
        }

        point_mesh->material() = the_material;
        point_mesh->material()->set_point_size(the_point_size);
        point_mesh->geometry()->vertices().clear();
        point_mesh->geometry()->append_vertices(the_points);
        point_mesh->geometry()->colors().resize(the_points.size(), gl::COLOR_WHITE);
        point_mesh->geometry()->point_sizes().resize(the_points.size(), 1.f);
        point_mesh->geometry()->create_gl_buffers();
        gl::draw_mesh(point_mesh);
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_texture(const gl::Texture &theTexture, const vec2 &theSize, const vec2 &theTopLeft,
                      const float the_brightness)
    {
        static gl::MaterialPtr material;

        // empty texture
        if(!theTexture){ return; }

        //create material, if not yet here
        if(!material)
        {
            try{ material = gl::Material::create(); }
            catch (Exception &e){LOG_ERROR<<e.what();}
            material->set_depth_test(false);
            material->set_depth_write(false);
            material->set_blending(true);
        }

#if !defined(KINSKI_GLES)
        static ShaderPtr tex_2D, rect_2D;

        // create shaders
        if(!tex_2D || !rect_2D)
        {
            tex_2D = material->shader();
            rect_2D = gl::create_shader(gl::ShaderType::RECT_2D);
        }

        if(theTexture.target() == GL_TEXTURE_2D){ material->set_shader(tex_2D); }
        else if(theTexture.target() == GL_TEXTURE_RECTANGLE)
        {
            material->set_shader(rect_2D);
            material->uniform("u_texture_size", theTexture.size());
        }
        else
        {
            LOG_ERROR << "drawTexture: texture target not supported";
            return;
        }
#endif
        // add the texture to the material
        material->clear_textures();
        material->set_diffuse(gl::Color(the_brightness, the_brightness, the_brightness, 1.f));
        material->add_texture(theTexture);

        vec2 sz = theSize;
        // flip to OpenGL coords
        vec2 tl = vec2(theTopLeft.x, g_viewport_dim[1] - theTopLeft.y);
        draw_quad(material, tl[0], tl[1], (tl+sz)[0], tl[1]-sz[1]);
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_texture_with_mask(const gl::Texture &the_texture,
                                const gl::Texture &the_mask,
                                const vec2 &theSize,
                                const vec2 &theTopLeft,
                                const float the_brightness)
    {
        static gl::MaterialPtr material;

        // empty texture
        if(!the_texture || !the_mask){ return; }

        //create material, if not yet here
        if(!material)
        {
            try{ material = gl::Material::create(gl::ShaderType::UNLIT_MASK); }
            catch (Exception &e){LOG_ERROR<<e.what();}
            material->set_depth_test(false);
            material->set_depth_write(false);
            material->set_blending(true);
        }

        // add the texture to the material
        material->set_textures({the_texture, the_mask});
        material->set_diffuse(gl::Color(the_brightness, the_brightness, the_brightness, 1.f));

        vec2 sz = theSize;
        // flip to OpenGL coords
        vec2 tl = vec2(theTopLeft.x, g_viewport_dim[1] - theTopLeft.y);
        draw_quad(material, tl[0], tl[1], (tl+sz)[0], tl[1]-sz[1]);
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_quad(const gl::Color &theColor,
                  const vec2 &theSize,
                  const vec2 &theTopLeft,
                  bool filled)
    {
        static gl::MaterialPtr material;

        //create material, if not yet here
        if(!material)
        {
            try{material = gl::Material::create();}
            catch (Exception &e){LOG_ERROR<<e.what();}
            material->set_depth_test(false);
            material->set_depth_write(false);
            material->set_blending(true);
        }
        material->set_diffuse(theColor);

        vec2 sz = theSize;
        // flip to OpenGL coords
        vec2 tl = vec2(theTopLeft.x, g_viewport_dim[1] - theTopLeft.y);
        draw_quad(material, tl[0], tl[1], (tl+sz)[0], tl[1]-sz[1], filled);
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_quad(const gl::MaterialPtr &theMaterial,
                  const vec2 &theSize,
                  const vec2 &theTl,
                  bool filled)
    {
        // flip to OpenGL coords
        vec2 tl = vec2(theTl.x, g_viewport_dim[1] - theTl.y);
        draw_quad(theMaterial, tl[0], tl[1], (tl + theSize)[0], tl[1] - theSize[1], filled);
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_quad(const gl::MaterialPtr &theMaterial,
                  float x0, float y0, float x1, float y1, bool filled)
    {
        // orthographic projection with a [0,1] coordinate space
        static MeshPtr quad_mesh;
        static mat4 projectionMatrix = ortho(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);

        if(!quad_mesh)
        {
            gl::GeometryPtr geom = gl::Geometry::create();
            geom->vertices().push_back(glm::vec3(-0.5f, 0.5f, 0.f));
            geom->vertices().push_back(glm::vec3(-0.5f, -0.5f, 0.f));
            geom->vertices().push_back(glm::vec3(0.5f, -0.5f, 0.f));
            geom->vertices().push_back(glm::vec3(0.5f, 0.5f, 0.f));
            geom->tex_coords().push_back(glm::vec2(0.f, 1.f));
            geom->tex_coords().push_back(glm::vec2(0.f, 0.f));
            geom->tex_coords().push_back(glm::vec2(1.f, 0.f));
            geom->tex_coords().push_back(glm::vec2(1.f, 1.f));
            geom->colors().assign(4, glm::vec4(1.f));
            geom->normals().assign(4, glm::vec3(0, 0, 1));
            geom->compute_bounding_box();
            geom->compute_tangents();
            quad_mesh = gl::Mesh::create(geom, Material::create());
            quad_mesh->set_position(glm::vec3(0.5f, 0.5f , 0.f));
        }
        quad_mesh->geometry()->set_primitive_type(filled ? GL_TRIANGLE_FAN : GL_LINE_LOOP);
        quad_mesh->material() = theMaterial;
        float scaleX = (x1 - x0) / g_viewport_dim[0];
        float scaleY = (y0 - y1) / g_viewport_dim[1];
        mat4 modelViewMatrix = glm::scale(mat4(), vec3(scaleX, scaleY, 1));
        modelViewMatrix[3] = vec4(x0 / g_viewport_dim[0], y1 / g_viewport_dim[1] , 0, 1);

        gl::ScopedMatrixPush model(MODEL_VIEW_MATRIX), projection(PROJECTION_MATRIX);
        gl::load_matrix(gl::PROJECTION_MATRIX, projectionMatrix);
        gl::load_matrix(gl::MODEL_VIEW_MATRIX, modelViewMatrix * quad_mesh->transform());
        draw_mesh(quad_mesh);
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_text_2D(const std::string &theText, const gl::Font &theFont, const Color &the_color,
                      const glm::vec2 &theTopLeft)
    {
        gl::ScopedMatrixPush model(MODEL_VIEW_MATRIX), projection(PROJECTION_MATRIX);

        if(!theFont.glyph_texture()) return;
        mat4 projectionMatrix = ortho(0.0f, g_viewport_dim[0], 0.0f, g_viewport_dim[1], 0.0f, 1.0f);

        // create the font mesh
        gl::MeshPtr m = theFont.create_mesh(theText, the_color);

        m->material()->set_diffuse(the_color);
        m->material()->set_depth_test(false);
        m->set_position(glm::vec3(theTopLeft.x, g_viewport_dim[1] - theTopLeft.y -
                                 m->geometry()->bounding_box().height(), 0.f));
        gl::load_matrix(gl::PROJECTION_MATRIX, projectionMatrix);
        gl::load_matrix(gl::MODEL_VIEW_MATRIX, m->transform());
        draw_mesh(m);
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_grid(float width, float height, int numW, int numH)
    {
        static std::map<std::tuple<float,float,int,int>, MeshPtr> theMap;
        static vec4 colorGrey(.7, .7, .7, 1.0), colorRed(1.0, 0, 0 ,1.0), colorBlue(0, 0, 1.0, 1.0);

        // search for incoming key
        auto conf = std::make_tuple(width, height, numW, numH);

        if(theMap.find(conf) == theMap.end())
        {
            auto mesh = gl::Mesh::create(gl::Geometry::create_grid(width, height, numW, numH),
                                         gl::Material::create());

//            theMap.clear();
            theMap[conf] = mesh;
        }
        draw_mesh(theMap[conf]);
    }

///////////////////////////////////////////////////////////////////////////////

void draw_transform(const glm::mat4& the_transform, float the_scale)
{
    static gl::MeshPtr transform_mesh;

    if(!transform_mesh)
    {
        transform_mesh = gl::Mesh::create(gl::Geometry::create(), gl::Material::create());
        auto &verts = transform_mesh->geometry()->vertices();
        auto &colors = transform_mesh->geometry()->colors();
        transform_mesh->geometry()->set_primitive_type(GL_LINES);
        verts =
        {
            glm::vec3(0), gl::X_AXIS,
            glm::vec3(0), gl::Y_AXIS,
            glm::vec3(0), gl::Z_AXIS
        };
        colors =
        {
            gl::COLOR_RED, gl::COLOR_RED,
            gl::COLOR_GREEN, gl::COLOR_GREEN,
            gl::COLOR_BLUE, gl::COLOR_BLUE
        };
    }
    gl::ScopedMatrixPush sp(gl::MODEL_VIEW_MATRIX);
    gl::mult_matrix(gl::MODEL_VIEW_MATRIX, glm::scale(the_transform, glm::vec3(the_scale)));
    gl::draw_mesh(transform_mesh);
}

///////////////////////////////////////////////////////////////////////////////

void draw_mesh(const MeshPtr &the_mesh, const ShaderPtr &overide_shader)
{
    KINSKI_CHECK_GL_ERRORS();
    if(!the_mesh || the_mesh->geometry()->vertices().empty()) return;

    const glm::mat4 &modelView = g_modelViewMatrixStack.top();
    mat4 mvp_matrix = g_projectionMatrixStack.top() * modelView;
    mat3 normal_matrix = glm::inverseTranspose(glm::mat3(modelView));

    for(auto &mat : the_mesh->materials())
    {
        mat->uniform("u_modelViewMatrix", modelView);
        mat->uniform("u_modelViewProjectionMatrix", mvp_matrix);
        if(the_mesh->geometry()->has_normals()){ mat->uniform("u_normalMatrix", normal_matrix); }
        if(the_mesh->geometry()->has_bones()){ mat->uniform("u_bones", the_mesh->bone_matrices()); }
    }
//  if(!the_mesh->geometry()->has_indices() || the_mesh->entries().empty())
    { gl::apply_material(the_mesh->material(), false, overide_shader); }
    
    the_mesh->bind_vertex_array(overide_shader ? overide_shader : the_mesh->materials()[0]->shader());
    
    if(the_mesh->geometry()->has_indices())
    {
        if(!the_mesh->entries().empty())
        {
            std::list<uint32_t> mat_entries[the_mesh->materials().size()];
            
            for (uint32_t i = 0; i < the_mesh->entries().size(); ++i)
            {
                uint32_t mat_index = the_mesh->entries()[i].material_index;
                
                if(mat_index < the_mesh->materials().size())
                    mat_entries[mat_index].push_back(i);
            }
            
            for (uint32_t i = 0; i < the_mesh->materials().size(); ++i)
            {
                const auto& entry_list = mat_entries[i];
                
                if(!entry_list.empty())
                {
                    if(!overide_shader){ the_mesh->bind_vertex_array(i); }
                    apply_material(the_mesh->materials()[i], false, overide_shader);
                }

                for (auto entry_index : entry_list)
                {
                    const gl::Mesh::Entry &e = the_mesh->entries()[entry_index];
                    
                    // skip disabled entries
                    if(!e.enabled) continue;
                    
                    uint32_t primitive_type = e.primitive_type;
                    primitive_type = primitive_type ? : the_mesh->geometry()->primitive_type();
#ifndef KINSKI_GLES
                    glDrawElementsBaseVertex(primitive_type,
                                             e.num_indices,
                                             the_mesh->geometry()->index_type(),
                                             BUFFER_OFFSET(e.base_index *the_mesh->geometry()->index_size()),
                                             e.base_vertex);
#else
                    glDrawElements(primitive_type,
                                   e.num_indices,
                                   the_mesh->geometry()->index_type(),
                                   BUFFER_OFFSET(e.base_index * the_mesh->geometry()->index_size()));
#endif
                }
            }
        }
        else

        {
            glDrawElements(the_mesh->geometry()->primitive_type(),
                           the_mesh->geometry()->indices().size(), the_mesh->geometry()->index_type(),
                           BUFFER_OFFSET(0));
        }
    }
    else
    {
        glDrawArrays(the_mesh->geometry()->primitive_type(), 0,
                     the_mesh->geometry()->vertices().size());
    }
    KINSKI_CHECK_GL_ERRORS();

#ifndef KINSKI_NO_VAO
    GL_SUFFIX(glBindVertexArray)(0);
#endif

    KINSKI_CHECK_GL_ERRORS();
}

///////////////////////////////////////////////////////////////////////////////

    void draw_light(const LightPtr &theLight)
    {
        static gl::MeshPtr directional_mesh, point_mesh, spot_mesh;

        gl::ScopedMatrixPush mat_push(gl::MODEL_VIEW_MATRIX);

        if(!directional_mesh)
        {
            directional_mesh = gl::Mesh::create(gl::Geometry::create(), gl::Material::create());
            point_mesh = gl::Mesh::create(gl::Geometry::create_sphere(1.f, 8),
                                          gl::Material::create());
            spot_mesh = gl::Mesh::create(gl::Geometry::create_cone(1.f, 1.f, 16),
                                         gl::Material::create());

            glm::mat4 rot_spot_mat = glm::rotate(glm::mat4(), glm::half_pi<float>(), gl::X_AXIS);

            for(auto &vert : spot_mesh->geometry()->vertices())
            {
                vert -= vec3(0, 1, 0);
                vert = (rot_spot_mat * glm::vec4(vert, 1.f)).xyz();
            }

            std::list<gl::MaterialPtr> mats =
            {
                directional_mesh->material(),
                point_mesh->material(),
                spot_mesh->material()
            };
            for (auto mat : mats){ mat->set_wireframe(); }

        }
        gl::MeshPtr light_mesh;
        float scale = theLight->max_distance(1.f / 2.f);

        switch (theLight->type())
        {
            case gl::Light::DIRECTIONAL:
                light_mesh = directional_mesh;
                break;

            case gl::Light::POINT:
                light_mesh = point_mesh;
                gl::mult_matrix(gl::MODEL_VIEW_MATRIX, glm::scale(theLight->global_transform(),
                                                                  vec3(scale)));
                break;

            case gl::Light::SPOT:
            {
                light_mesh = spot_mesh;
                float r_scale = tan(glm::radians(theLight->spot_cutoff())) * scale;
                gl::mult_matrix(gl::MODEL_VIEW_MATRIX, glm::scale(theLight->global_transform(),
                                                                  vec3(r_scale, r_scale, scale)));
            }
                break;
        }

        if(theLight->enabled())
        {
            light_mesh->material()->set_diffuse(theLight->diffuse());
        }
        else
        {
//            light_mesh->material()->set_diffuse(gl::COLOR_RED);
            return;
        }

        // draw the configured mesh
        gl::draw_mesh(light_mesh);
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_boundingbox(const Object3DPtr &the_obj)
    {
        static vec4 colorWhite(1.0), colorRed(1.0, 0, 0 ,1.0);
        static MeshPtr line_mesh;
        if(!the_obj) return;

        if(!line_mesh)
        {
            line_mesh = gl::Mesh::create(gl::Geometry::create_box_lines());
        }
        AABB mesh_bb = the_obj->bounding_box();
        glm::mat4 center_mat = glm::translate(glm::mat4(), mesh_bb.center());

        glm::mat4 scale_mat = glm::scale(glm::mat4(), vec3(mesh_bb.width(),
                                                           mesh_bb.height(),
                                                           mesh_bb.depth()));


        gl::ScopedMatrixPush sp(gl::MODEL_VIEW_MATRIX);
        gl::mult_matrix(gl::MODEL_VIEW_MATRIX, center_mat * scale_mat);
        gl::draw_mesh(line_mesh);
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_normals(const MeshWeakPtr &the_mesh)
    {
        static map<MeshWeakPtr, MeshPtr, std::owner_less<MeshWeakPtr> > theMap;
        static vec4 colorGrey(.7, .7, .7, 1.0), colorRed(1.0, 0, 0 ,1.0), colorBlue(0, 0, 1.0, 1.0);

        if(theMap.find(the_mesh) == theMap.end())
        {
            Mesh::ConstPtr m = the_mesh.lock();
            if(m->geometry()->normals().empty()) return;
            GeometryPtr geom = Geometry::create();
            geom->set_primitive_type(GL_LINES);
            gl::MaterialPtr mat = gl::Material::create();
            MeshPtr line_mesh = gl::Mesh::create(geom, mat);
            vector<vec3> &thePoints = geom->vertices();
            vector<vec4> &theColors = geom->colors();
            const vector<vec3> &vertices = m->geometry()->vertices();
            const vector<vec3> &normals = m->geometry()->normals();

            float length = (m->geometry()->bounding_box().max -
                            m->geometry()->bounding_box().min).length() * 5;

            for (uint32_t i = 0; i < vertices.size(); i++)
            {
                thePoints.push_back(vertices[i]);
                thePoints.push_back(vertices[i] + normals[i] * length);
                theColors.push_back(colorGrey);
                theColors.push_back(colorRed);
            }
//            geom->create_gl_buffers();
//            line_mesh->create_vertex_array();
            theMap[the_mesh] = line_mesh;
        }
        gl::draw_mesh(theMap[the_mesh]);

        // cleanup
        map<MeshWeakPtr, MeshPtr >::iterator meshIt = theMap.begin();
        for (; meshIt != theMap.end(); ++meshIt)
        {
            if(! meshIt->first.lock() )
                theMap.erase(meshIt);
        }
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_circle(const vec2 &center, float radius, const gl::Color &the_color, bool solid,
                     int numSegments)
    {
        static gl::MaterialPtr color_mat;

        if(!color_mat)
        {
            color_mat = gl::Material::create();
            color_mat->set_depth_test(false);
            color_mat->set_depth_write(false);
        }
        color_mat->set_diffuse(the_color);
        draw_circle(center, radius, solid, color_mat, numSegments);
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_circle(const glm::vec2 &center, float radius, bool solid,
                     const MaterialPtr &theMaterial, int numSegments)
    {
        static gl::MeshPtr solid_mesh, line_mesh;
        static gl::MaterialPtr default_mat;
        static int last_num_sequments = 0;
        gl::MeshPtr our_mesh = solid ? solid_mesh : line_mesh;

        if(!our_mesh || last_num_sequments != numSegments)
        {
            last_num_sequments = numSegments;
            GeometryPtr geom = solid ? Geometry::create_solid_circle(numSegments) :
                Geometry::create_circle(numSegments);
            default_mat = gl::Material::create();
            default_mat->set_depth_test(false);
            default_mat->set_depth_write(false);
            our_mesh = gl::Mesh::create(geom, default_mat);
            if(solid)
                solid_mesh = our_mesh;
            else
                line_mesh = our_mesh;
        }
        our_mesh->material() = theMaterial ? theMaterial : default_mat;
        mat4 projectionMatrix = ortho(0.0f, g_viewport_dim[0], 0.0f, g_viewport_dim[1], 0.0f, 1.0f);
        mat4 modelView = glm::scale(mat4(), vec3(radius));
        modelView[3] = vec4(center.x, g_viewport_dim[1] - center.y, 0, modelView[3].w);

        ScopedMatrixPush m(MODEL_VIEW_MATRIX), p(PROJECTION_MATRIX);
        load_matrix(PROJECTION_MATRIX, projectionMatrix);
        load_matrix(MODEL_VIEW_MATRIX, modelView);
        draw_mesh(our_mesh);
    }

///////////////////////////////////////////////////////////////////////////////

    KINSKI_API gl::Texture render_to_texture(const gl::SceneConstPtr &theScene,
                                             gl::Fbo &theFbo,
                                             const gl::CameraPtr &theCam)
    {
        if(!theFbo)
        {
            LOG_WARNING << "trying to use an uninitialized FBO";
            return gl::Texture();
        }

        // push framebuffer and viewport states
        gl::SaveViewPort sv; gl::SaveFramebufferBinding sfb;
        gl::set_window_dimension(theFbo.size());
        theFbo.bind();
        gl::clear();
        theScene->render(theCam);
        return theFbo.texture();
    }

//#ifdef KINSKI_CPP11

    KINSKI_API gl::Texture render_to_texture(gl::Fbo &theFbo, std::function<void()> functor)
    {
        if(!theFbo)
        {
            LOG_WARNING << "trying to use an uninitialized FBO";
            return gl::Texture();
        }
        // push framebuffer and viewport states
        gl::SaveViewPort sv; gl::SaveFramebufferBinding sfb;
        gl::set_window_dimension(theFbo.size());
        theFbo.bind();
        functor();
        return theFbo.texture();
    }

//#endif

///////////////////////////////////////////////////////////////////////////////

    Texture create_cube_texture(const std::vector<gl::Texture> &the_planes)
    {
        gl::Texture ret;

#if !defined(KINSKI_GLES)
        // check if number and sizes of input textures match
        if(the_planes.size() != 6)
        {
            LOG_WARNING << "cube map creation failed. number of input textures must be 6 -- "
            << the_planes.size() << " provided";
            return ret;
        }

        auto tex_sz = the_planes.front().size();
        auto tex_target = the_planes.front().target();

        for (auto &t : the_planes)
        {
            if(!t || tex_sz != t.size() || tex_target != t.target())
            {
                LOG_WARNING << "cube map creation failed. size/type of input textures not consistent";
                return ret;
            }
        }

        gl::Buffer pixel_buf = gl::Buffer(GL_PIXEL_UNPACK_BUFFER, GL_STATIC_COPY);
        pixel_buf.set_data(nullptr, tex_sz.x * tex_sz.y * 4);

        GLuint tex_name;
        glGenTextures(1, &tex_name);

        pixel_buf.bind(GL_PIXEL_UNPACK_BUFFER);
        pixel_buf.bind(GL_PIXEL_PACK_BUFFER);

        for (uint32_t i = 0; i < 6; i++)
        {
            const gl::Texture &t = the_planes[i];
            t.bind();

            // copy data to PBO
            glGetTexImage(t.target(), 0, t.internal_format(), GL_UNSIGNED_BYTE, BUFFER_OFFSET(0));

            // bind cube map
            glBindTexture(GL_TEXTURE_CUBE_MAP, tex_name);

            // copy data from PBO to appropriate image plane
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, t.internal_format(), t.width(),
                         t.height(), 0, t.internal_format(), GL_UNSIGNED_BYTE, BUFFER_OFFSET(0));
        }
        pixel_buf.unbind(GL_PIXEL_UNPACK_BUFFER);
        pixel_buf.unbind(GL_PIXEL_PACK_BUFFER);

        ret = gl::Texture(GL_TEXTURE_CUBE_MAP, tex_name, tex_sz.x, tex_sz.y, false);
#endif
        return ret;
    }

///////////////////////////////////////////////////////////////////////////////

    void apply_material(const MaterialPtr &the_mat, bool force_apply, const ShaderPtr &override_shader)
    {
        KINSKI_CHECK_GL_ERRORS();
        static MaterialWeakPtr weak_last;
        
        MaterialPtr last_mat = force_apply ? MaterialPtr() : weak_last.lock();

        if(!the_mat) return;

        // weak copy of current mat
        weak_last = the_mat;

        // process load queues
        // texture queue
        if(!the_mat->texture_paths().empty())
        {
            std::list<gl::Texture> tmp_textures;
            
            for(auto &pair : the_mat->texture_paths())
            {
                if(pair.second == gl::Material::AssetLoadStatus::NOT_LOADED)
                {
                    try
                    {
                        tmp_textures.push_back(gl::create_texture_from_file(pair.first, true, true));
                        pair.second = gl::Material::AssetLoadStatus::LOADED;
                    }
                    catch(Exception &e)
                    {
                        LOG_WARNING << e.what();
                        pair.second = gl::Material::AssetLoadStatus::NOT_FOUND;
                    }
                }
            }
            the_mat->set_textures(concat_containers<gl::Texture>(tmp_textures, the_mat->textures()));
        }

        // shader queue
        for(auto &sh_type : the_mat->load_queue_shader())
        {
            try{ the_mat->set_shader(gl::create_shader(sh_type)); }
            catch(Exception &e){ LOG_WARNING << e.what(); }

        }
        the_mat->load_queue_shader().clear();
        if(!the_mat->shader()){ the_mat->set_shader(gl::create_shader(gl::ShaderType::UNLIT)); }

        // bind the shader
        gl::ShaderPtr shader = override_shader ? override_shader : the_mat->shader();
        shader->bind();
        KINSKI_CHECK_GL_ERRORS();
        
        char buf[512];

        // twoSided
        if(!last_mat || (last_mat->culling() != the_mat->culling() ||
                         last_mat->wireframe() != the_mat->wireframe()))
        {
            if(the_mat->culling() == Material::CULL_NONE || the_mat->wireframe())
            { glDisable(GL_CULL_FACE); }
            else
            {
                glEnable(GL_CULL_FACE);
                int val = GL_BACK;

                if(the_mat->culling() & Material::CULL_BACK)
                {
                    val = GL_BACK;

                    if(the_mat->culling() & Material::CULL_FRONT){ val = GL_FRONT_AND_BACK; }
                }else if(the_mat->culling() & Material::CULL_FRONT){ val = GL_FRONT; }
                glCullFace(val);
            }
        }
        KINSKI_CHECK_GL_ERRORS();

        // wireframe ?
#ifndef KINSKI_GLES
        if(!last_mat || last_mat->wireframe() != the_mat->wireframe())
            glPolygonMode(GL_FRONT_AND_BACK, the_mat->wireframe() ? GL_LINE : GL_FILL);
#endif

        KINSKI_CHECK_GL_ERRORS();

        // read write depth buffer ?
        if(!last_mat || last_mat->depth_test() != the_mat->depth_test())
        {
            if(the_mat->depth_test()) { glEnable(GL_DEPTH_TEST); }
            else { glDisable(GL_DEPTH_TEST); }
        }
        KINSKI_CHECK_GL_ERRORS();

        if(!last_mat || last_mat->depth_write() != the_mat->depth_write())
        {
            if(the_mat->depth_write()) glDepthMask(GL_TRUE);
            else glDepthMask(GL_FALSE);
        }
        KINSKI_CHECK_GL_ERRORS();

        if(!last_mat || last_mat->stencil_test() != the_mat->stencil_test())
        {
            if(the_mat->stencil_test()){ glEnable(GL_STENCIL_TEST); }
            else{ glDisable(GL_STENCIL_TEST); }

        }
        KINSKI_CHECK_GL_ERRORS();

        if(!last_mat || last_mat->blending() != the_mat->blending()
                     || last_mat->blend_factors() != the_mat->blend_factors()
                     || last_mat->blend_equation() != the_mat->blend_equation())
        {
            if(!the_mat->blending())
            {
                glDisable(GL_BLEND);
            }
            else
            {
                glEnable(GL_BLEND);
                glBlendFunc(the_mat->blend_src(), the_mat->blend_dst());
#if !defined(KINSKI_GLES_2)
                glBlendEquation(the_mat->blend_equation());
#endif
            }
        }
        KINSKI_CHECK_GL_ERRORS();

        if(!last_mat || last_mat->point_size() != the_mat->point_size())
        {
            if(the_mat->point_size() > 0.f)
            {
#ifndef KINSKI_GLES
                glEnable(GL_PROGRAM_POINT_SIZE);
                glPointSize(the_mat->point_size());
#endif
                KINSKI_CHECK_GL_ERRORS();
            }
        }

#if defined(KINSKI_GLES)
        if(!last_mat || last_mat->line_width() != the_mat->line_width())
        {
//            if(the_mat->line_width() > 1.f)
//            {
//                glEnable(GL_LINE_SMOOTH);
                glLineWidth(the_mat->line_width());
                KINSKI_CHECK_GL_ERRORS();
//            }
        }
#endif
        
        if(!last_mat || override_shader)
        {
        
        }
        
        // texture matrix from first texture, if any
        shader->uniform("u_textureMatrix", (the_mat->textures().empty() || !the_mat->textures().front()) ?
                                          glm::mat4() : the_mat->textures().front().texture_matrix());

        shader->uniform("u_numTextures", (GLint) the_mat->textures().size());

//        if(the_mat->textures().empty()) glBindTexture(GL_TEXTURE_2D, 0);

        // add texturemaps
        int32_t tex_unit = 0, tex_2d = 0;
#if !defined(KINSKI_GLES_2)
        int32_t tex_3d = 0, tex_2d_array = 0, tex_cube = 0;
#endif
        
#if !defined(KINSKI_GLES)
        int32_t tex_rect = 0;
#endif

        for(auto &t : the_mat->textures())
        {
            if(!t){ continue; }

            t.bind(tex_unit);

            switch (t.target())
            {
                case GL_TEXTURE_2D:
                    sprintf(buf, "u_sampler_2D[%d]", tex_2d++);
                    break;

#if !defined(KINSKI_GLES_2)

                case GL_TEXTURE_3D:
                    sprintf(buf, "u_sampler_3D[%d]", tex_3d++);
                    break;

                case GL_TEXTURE_2D_ARRAY:
                    sprintf(buf, "u_sampler_2D_array[%d]", tex_2d_array++);
                    break;
                
                case GL_TEXTURE_CUBE_MAP:
                    sprintf(buf, "u_sampler_cube[%d]", tex_cube++);
                    break;
#endif
                    
#if !defined(KINSKI_GLES)
                    
                case GL_TEXTURE_RECTANGLE:
                    sprintf(buf, "u_sampler_2Drect[%d]", tex_rect++);
                    break;
#endif
                default:
                    break;
            }
            the_mat->uniform(buf, tex_unit);
            tex_unit++;
        }

        KINSKI_CHECK_GL_ERRORS();

        // update uniform buffers and uniform values for current shader
        the_mat->update_uniforms(shader);
    }

///////////////////////////////////////////////////////////////////////////////

    void reset_state()
    {
        static auto mat = gl::Material::create();
        gl::apply_material(mat, true);
    }

///////////////////////////////////////////////////////////////////////////////

    const std::set<std::string>& get_extensions()
    {
        static std::set<std::string> s_extensions;

        if(s_extensions.empty())
        {
#if !defined(KINSKI_GLES_2)
            GLint numExtensions = 0;
            glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions) ;
            for (int i = 0; i < numExtensions; ++i) {
                s_extensions.insert((char*)glGetStringi(GL_EXTENSIONS, i)) ;
            }
#endif
        }
        return s_extensions;
    }

///////////////////////////////////////////////////////////////////////////////

    bool is_extension_supported(const std::string &theName)
    {
        return get_extensions().find(theName) != get_extensions().end();
    }

///////////////////////////////////////////////////////////////////////////////

    // SaveFramebufferBinding
    SaveFramebufferBinding::SaveFramebufferBinding()
    {
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_old_value );
    }

    SaveFramebufferBinding::~SaveFramebufferBinding()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_old_value );
    }

///////////////////////////////////////////////////////////////////////////////

    bool is_point_inside_mesh(const glm::vec3& p, gl::MeshPtr m)
    {
        // checks only make sense with triangle geometry
        if(!m ||
           m->geometry()->primitive_type() != GL_TRIANGLES ||
           m->geometry()->faces().empty())
        {
            return false;
        }

        auto aabb = m->bounding_box();

        // checks if p is inside the aabb of our mesh)
        if(!aabb.contains(p)) return false;

        const auto &vertices = m->geometry()->vertices();

        // check the point's distance to all triangle planes
        for (const auto &face : m->geometry()->faces())
        {
            gl::Plane plane(vertices[face.a], vertices[face.b], vertices[face.c]);
            plane.transform(m->transform());
            if(plane.distance(p) < 0)
                return false;
        }
        return true;
    }

///////////////////////////////////////////////////////////////////////////////

    void project_texcoords(gl::MeshPtr src, gl::MeshPtr dest)
    {
        const auto &src_verts = src->geometry()->vertices();
        const auto &src_texcoords = src->geometry()->tex_coords();
        const auto &dest_verts = dest->geometry()->vertices();
        const auto &dest_normals = dest->geometry()->normals();
        auto &dest_texcoords = dest->geometry()->tex_coords();

        // aquire enough space for texcoords
        dest_texcoords.resize(dest_verts.size());

        // helper structure and comparator for sorting
        struct hit_struct
        {
            gl::Face3 face;
            float u, v, distance;
        };
        auto hit_struct_comp = [](const hit_struct &h1, const hit_struct &h2) -> bool
        {
            return h1.distance < h2.distance;
        };

        float ray_offset = 2 * glm::length(src->bounding_box().transform(src->global_transform()).halfExtents());
        float scale_val = 1.01f;
        mat4 world_to_src = glm::inverse(glm::scale(src->global_transform(), vec3(scale_val)));

        for(uint32_t i = 0; i < dest_verts.size(); i++)
        {
            gl::Ray ray(dest_verts[i] + dest_normals[i] * ray_offset, -dest_normals[i]);
            ray = ray.transform(dest->transform());
            gl::Ray ray_in_object_space = ray.transform(world_to_src);

            std::vector<hit_struct> hit_structs;

            for (const auto &face : src->geometry()->faces())
            {
                gl::Triangle t(src_verts[face.a], src_verts[face.b], src_verts[face.c]);

                if(gl::ray_triangle_intersection ray_tri_hit = t.intersect(ray_in_object_space))
                {
                    hit_structs.push_back({face, ray_tri_hit.u, ray_tri_hit.v, ray_tri_hit.distance});
                }
            }
            if(!hit_structs.empty())
            {
                std::sort(hit_structs.begin(), hit_structs.end(), hit_struct_comp);
                const auto & hs = hit_structs.front();
                float u, v, w;
                u = hs.u, v = hs.v, w = 1 - u - v;

                dest_texcoords[i] = src_texcoords[hs.face.a] * v +
                src_texcoords[hs.face.b] * u +
                src_texcoords[hs.face.c] * w;

                dest_texcoords[i] = dest_texcoords[i].yx();
                dest_texcoords[i].x = 1 - dest_texcoords[i].x;
                dest_texcoords[i].y = 1 - dest_texcoords[i].y;

            }else{ LOG_ERROR << "no triangle hit"; }
        }
    }

///////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // global shader creation functions

    ShaderPtr create_shader_from_file(const std::string &vertPath,
                                      const std::string &fragPath,
                                      const std::string &geomPath)
    {
        ShaderPtr ret;
        std::string vertSrc, fragSrc, geomSrc;
        vertSrc = fs::read_file(vertPath);
        fragSrc = fs::read_file(fragPath);

        if (!geomPath.empty()) geomSrc = fs::read_file(geomPath);

        try { ret = gl::Shader::create(vertSrc, fragSrc, geomSrc); }
        catch (Exception &e){ LOG_ERROR<<e.what(); }
        return ret;
    }

    ShaderPtr create_shader(ShaderType type, bool use_cached_shader)
    {
        ShaderPtr ret;
        auto it = use_cached_shader ? g_shaders.find(type) : g_shaders.end();

        if(it == g_shaders.end())
        {
            std::string vert_src, frag_src, geom_src;

            switch (type)
            {
                case ShaderType::UNLIT:
                    vert_src = unlit_vert;
                    frag_src = unlit_frag;
                    break;

                case ShaderType::UNLIT_MASK:
                    vert_src = unlit_vert;
                    frag_src = unlit_mask_frag;
                    break;

                case ShaderType::SDF_FONT:
                    vert_src = unlit_vert;
                    frag_src = distance_field_frag;
                    break;

                case ShaderType::UNLIT_SKIN:
                    vert_src = unlit_skin_vert;
                    frag_src = unlit_frag;
                    break;

                case ShaderType::GOURAUD:
                    vert_src = gouraud_vert;
                    frag_src = gouraud_frag;
                    break;

                case ShaderType::PHONG:
                    vert_src = phong_vert;
                    frag_src = phong_frag;
                    break;

                case ShaderType::NOISE_3D:
                    vert_src = unlit_vert;
                    frag_src = noise_3D_frag;
                    break;

                case ShaderType::POINTS_COLOR:
                case ShaderType::POINTS_TEXTURE:
                    vert_src = points_vert;
                    frag_src = points_frag;
                    break;

#if !defined(KINSKI_GLES)

                case ShaderType::BLUR:
                    vert_src = unlit_vert;
                    frag_src = blur_poisson_frag;
                    break;

                case ShaderType::DEPTH_OF_FIELD:
                    vert_src = unlit_vert;
                    frag_src = depth_of_field_frag;
                    break;

                case ShaderType::PHONG_SHADOWS:
                    vert_src = phong_shadows_vert;
                    frag_src = phong_shadows_frag;
                    break;
                case ShaderType::PHONG_SKIN_SHADOWS:
                    vert_src = phong_skin_shadows_vert;
                    frag_src = phong_shadows_frag;
                    break;

                case ShaderType::RECT_2D:
                    vert_src = unlit_rect_vert;
                    frag_src = unlit_rect_frag;
                    break;

                case ShaderType::PHONG_NORMALMAP:
                    vert_src = phong_normalmap_vert;
                    frag_src = phong_normalmap_frag;
                    break;

                case ShaderType::PHONG_SKIN:
                    vert_src = phong_skin_vert;
                    frag_src = phong_frag;
                    break;

                case ShaderType::LINES_2D:
                    vert_src = unlit_vert;
                    frag_src = unlit_frag;
                    geom_src = lines_2D_geom;
                    break;

                case ShaderType::POINTS_SPHERE:
                    vert_src = points_vert;
                    frag_src = points_sphere_frag;
                    break;
#endif
                default:
                    break;
            }

            if(vert_src.empty() || frag_src.empty())
            {
                LOG_WARNING << get_shader_name(type) << " not available, falling back to: " <<
                    get_shader_name(ShaderType::UNLIT);
                return create_shader(gl::ShaderType::UNLIT, false);
            }
            ret = gl::Shader::create(vert_src, frag_src, geom_src);
            if(use_cached_shader){ g_shaders[type] = ret; }
            KINSKI_CHECK_GL_ERRORS();
        }
        else{ ret = it->second; }
        return ret;
    }
    
    const std::string& get_shader_name(ShaderType the_type)
    {
        auto it = g_shader_names.find(the_type);
        if(it != g_shader_names.end()){ return it->second; }
        return g_shader_names[ShaderType::UNKNOWN];
    }
    
}}//namespace
