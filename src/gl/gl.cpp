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
        std::unique_ptr<Context> g_context;
        
        std::map<gl::ShaderType, std::string> g_shader_names =
        {
            {ShaderType::NONE, "NONE"},
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

        using vao_map_key_t = std::pair<const gl::Geometry*, const gl::Shader*>;
        using vao_map_t = std::unordered_map<vao_map_key_t, uint32_t, PairHash<const gl::Geometry*, const gl::Shader*>>;

        using fbo_map_key_t = std::pair<const gl::Fbo*, uint32_t>;
        using fbo_map_t = std::unordered_map<fbo_map_key_t, uint32_t, PairHash<const gl::Fbo*, uint32_t>>;
    };

    struct ContextImpl
    {
        std::shared_ptr<PlatformData> m_platform_data;
        void* m_current_context_id = nullptr;
        std::unordered_map<void*, vao_map_t> m_vao_maps;
        std::unordered_map<void*, fbo_map_t> m_fbo_maps;
    };

    Context::Context(std::shared_ptr<PlatformData> platform_data):m_impl(new ContextImpl)
    {
        m_impl->m_platform_data = platform_data;
    }

    std::shared_ptr<PlatformData> Context::platform_data()
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

    uint32_t Context::get_vao(const gl::GeometryPtr &the_geom, const gl::ShaderPtr &the_shader)
    {
        vao_map_t& vao_map = m_impl->m_vao_maps[m_impl->m_current_context_id];
        auto it = vao_map.find(vao_map_key_t(the_geom.get(), the_shader.get()));
        if(it != vao_map.end()){ return it->second; }
        return 0;
    }

    uint32_t Context::create_vao(const gl::GeometryPtr &the_geom, const gl::ShaderPtr &the_shader)
    {
#ifndef KINSKI_NO_VAO
        auto key = vao_map_key_t(the_geom.get(), the_shader.get());
        vao_map_t& vao_map = m_impl->m_vao_maps[m_impl->m_current_context_id];
        auto it = vao_map.find(key);

        // delete old vao
        if(it != vao_map.end()){ GL_SUFFIX(glDeleteVertexArrays)(1, &it->second); }

        // create new vao
        uint32_t vao_id = 0;
        GL_SUFFIX(glGenVertexArrays)(1, &vao_id);
        vao_map[key] = vao_id;
        return vao_id;
#endif
        return 0;
    }

    void Context::clear_vao(const gl::GeometryPtr &the_geom, const gl::ShaderPtr &the_shader)
    {
        auto key = vao_map_key_t(the_geom.get(), the_shader.get());
        vao_map_t& vao_map = m_impl->m_vao_maps[m_impl->m_current_context_id];
        vao_map.erase(key);
    }

    uint32_t Context::get_fbo(const gl::Fbo* the_fbo, uint32_t the_index)
    {
        auto key = fbo_map_key_t(the_fbo, the_index);
        fbo_map_t &fbo_map = m_impl->m_fbo_maps[m_impl->m_current_context_id];
        auto it = fbo_map.find(key);
        if(it != fbo_map.end()){ return it->second; }
        return 0;
    }

    uint32_t Context::create_fbo(const gl::Fbo* the_fbo, uint32_t the_index)
    {
        auto key = fbo_map_key_t(the_fbo, the_index);
        fbo_map_t &fbo_map = m_impl->m_fbo_maps[m_impl->m_current_context_id];
        auto it = fbo_map.find(key);

        // delete old fbo
        if(it != fbo_map.end()){ glDeleteFramebuffers(1, &it->second); }

        // create new vao
        uint32_t fbo_id = 0;
        glGenFramebuffers(1, &fbo_id);
        fbo_map[key] = fbo_id;
        return fbo_id;
    }

    void Context::clear_fbo(const gl::Fbo* the_fbo, uint32_t the_index)
    {
        auto key = fbo_map_key_t(the_fbo, the_index);
        fbo_map_t &fbo_map = m_impl->m_fbo_maps[m_impl->m_current_context_id];
        auto it = fbo_map.find(key);

        // delete fbo
        if(it != fbo_map.end()){ glDeleteFramebuffers(1, &it->second); }

        fbo_map.erase(key);
    }

    void Context::clear_assets_for_context(void* the_context_id)
    {
        m_impl->m_vao_maps.erase(the_context_id);
        m_impl->m_fbo_maps.erase(the_context_id);
    }

    void create_context(const std::shared_ptr<PlatformData> &the_platform_data)
    {
        g_context.reset(new Context(the_platform_data));
    }

    const std::unique_ptr<Context>& context(){ return g_context; }

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

        }else if (OrthoCamera::Ptr cam = dynamic_pointer_cast<OrthoCamera>(theCamera))
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

        geom->colors().resize(geom->vertices().size(), gl::COLOR_WHITE);
        geom->compute_aabb();
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
                cam = gl::OrthoCamera::create(-v, v, -v, v, .1f, far_clip);
                break;
            }
            case gl::Light::POINT:
                cam = gl::CubeCamera::create(.1f, far_clip);
                break;
            case gl::Light::SPOT:
                cam = gl::PerspectiveCamera::create(1.f, 2 * the_light->spot_cutoff(),
                                                    .1f, far_clip);
                break;
                
            default:
                LOG_WARNING << "light type not handled";
                return nullptr;
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

    void clear_color(const Color &the_color)
    {
        glClearColor(the_color.r, the_color.g, the_color.b, the_color.a);
    }

///////////////////////////////////////////////////////////////////////////////

    void clear()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }

///////////////////////////////////////////////////////////////////////////////
    
    void clear(const gl::Color &the_color)
    {
        gl::Color c;
        glGetFloatv(GL_COLOR_CLEAR_VALUE, &c[0]);
        if(the_color != c){ glClearColor(the_color.r, the_color.g, the_color.b, the_color.a); }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        if(the_color != c){ glClearColor(c.r, c.g, c.b, c.a); }
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
            mesh->material()->set_shader(gl::create_shader(gl::ShaderType::LINES_2D));
        }
        mesh->material()->set_diffuse(theColor);
        mesh->material()->set_line_width(line_thickness);
        mesh->material()->uniform("u_window_size", window_dimension());
        mesh->material()->uniform("u_line_thickness", line_thickness);
        mesh->geometry()->append_vertices(thePoints);
        mesh->geometry()->colors().resize(thePoints.size(), gl::COLOR_WHITE);
        mesh->create_vertex_attribs(true);
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
        point_mesh->geometry()->vertices() = the_points;
        point_mesh->geometry()->colors().resize(the_points.size(), gl::COLOR_WHITE);
        point_mesh->geometry()->point_sizes().resize(the_points.size(), 1.f);
        point_mesh->create_vertex_attribs(true);
        gl::draw_mesh(point_mesh);
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_texture(const gl::Texture &theTexture, const vec2 &theSize, const vec2 &theTopLeft,
                      const float the_brightness, const float the_gamma)
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
        material->uniform("u_gamma", the_gamma);
#endif
        // add the texture to the material
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
                                const float the_brightness,
                                const float the_gamma)
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
        material->add_texture(the_texture);
        material->add_texture(the_mask, Texture::Usage::MASK);
        material->set_diffuse(gl::Color(the_brightness, the_brightness, the_brightness, 1.f));
        material->uniform("u_gamma", the_gamma);

        vec2 sz = theSize;
        // flip to OpenGL coords
        vec2 tl = vec2(theTopLeft.x, g_viewport_dim[1] - theTopLeft.y);
        draw_quad(material, tl[0], tl[1], (tl+sz)[0], tl[1]-sz[1]);
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_quad(const vec2 &the_size, const Color &the_color, const vec2 &the_topleft, bool fill)
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
        material->set_diffuse(the_color);

        vec2 sz = the_size;
        // flip to OpenGL coords
        vec2 tl = vec2(the_topleft.x, g_viewport_dim[1] - the_topleft.y);
        draw_quad(material, tl[0], tl[1], (tl+sz)[0], tl[1]-sz[1], fill);
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_quad(const vec2 &the_size, const MaterialPtr &the_material, const vec2 &the_topleft,
                   bool fill)
    {
        // flip to OpenGL coords
        vec2 tl = vec2(the_topleft.x, g_viewport_dim[1] - the_topleft.y);
        draw_quad(the_material, tl[0], tl[1], (tl + the_size)[0], tl[1] - the_size[1], fill);
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
            geom->compute_aabb();
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
                                 m->geometry()->aabb().height(), 0.f));
        gl::load_matrix(gl::PROJECTION_MATRIX, projectionMatrix);
        gl::load_matrix(gl::MODEL_VIEW_MATRIX, m->transform());
        draw_mesh(m);
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_grid(float width, float height, int numW, int numH)
    {
        static std::map<std::tuple<int, int>, MeshPtr> theMap;

        // search for incoming key
        auto conf = std::make_tuple(numW, numH);
        auto it = theMap.find(conf);
        
        if(it == theMap.end())
        {
            auto mesh = gl::Mesh::create(gl::Geometry::create_grid(1.f, 1.f, numW, numH),
                                         gl::Material::create());
            it = theMap.insert(std::make_pair(conf, mesh)).first;
        }
        gl::ScopedMatrixPush sp(gl::MODEL_VIEW_MATRIX);
        gl::mult_matrix(gl::MODEL_VIEW_MATRIX, glm::scale(glm::mat4(), gl::vec3(width, 1.f, height)));
        draw_mesh(it->second);
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
    if(!the_mesh) return;

    matrix_struct_140_t m;
    m.model_view = g_modelViewMatrixStack.top();
    m.model_view_projection = g_projectionMatrixStack.top() * m.model_view;
    m.normal_matrix = mat4(glm::inverseTranspose(glm::mat3(m.model_view)));
    m.texture_matrix = the_mesh->material()->texture_matrix();

#if !defined(KINSKI_GLES)
    static gl::Buffer matrix_ubo(GL_UNIFORM_BUFFER, GL_STREAM_DRAW);
    matrix_ubo.set_data(&m, sizeof(matrix_struct_140_t));
    glBindBufferBase(GL_UNIFORM_BUFFER, gl::Context::MATRIX_BLOCK, matrix_ubo.id());
#endif

    for(auto &mat : the_mesh->materials())
    {
#if !defined(KINSKI_GLES)
        auto &shader = overide_shader ? overide_shader : mat->shader();
        shader->uniform_block_binding("MatrixBlock", gl::Context::MATRIX_BLOCK);
        shader->uniform_block_binding("LightBlock", gl::Context::LIGHT_BLOCK);
#else
        mat->uniform("u_modelViewMatrix", modelView);
        mat->uniform("u_modelViewProjectionMatrix", mvp_matrix);
        if(the_mesh->geometry()->has_normals()){ mat->uniform("u_normalMatrix", normal_matrix); }
#endif
        mat->uniform("u_window_dimension", gl::window_dimension());
        if(the_mesh->geometry()->has_bones()){ mat->uniform("u_bones", the_mesh->bone_matrices()); }
    }
    if(the_mesh->geometry()->has_dirty_buffers()){ the_mesh->geometry()->create_gl_buffers(); }

    std::map<ImagePtr, gl::Texture> img_tex_map;
    gl::apply_material(the_mesh->material(), false, overide_shader, &img_tex_map);
    KINSKI_CHECK_GL_ERRORS();

    the_mesh->bind_vertex_array(overide_shader ? overide_shader : the_mesh->materials()[0]->shader());
    KINSKI_CHECK_GL_ERRORS();
    
    if(the_mesh->geometry()->has_indices() || the_mesh->index_buffer())
    {
        if(!the_mesh->entries().empty())
        {
            std::list<uint32_t> mat_entries[the_mesh->materials().size()];
            
            for (uint32_t i = 0; i < the_mesh->entries().size(); ++i)
            {
                uint32_t mat_index = the_mesh->entries()[i].material_index;
                if(mat_index < the_mesh->materials().size()){ mat_entries[mat_index].push_back(i); }
            }
            
            for (uint32_t i = 0; i < the_mesh->materials().size(); ++i)
            {
                const auto& entry_list = mat_entries[i];
                
                if(!entry_list.empty())
                {
                    if(!overide_shader){ the_mesh->bind_vertex_array(i); }
                    apply_material(the_mesh->materials()[i], false, overide_shader, &img_tex_map);
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
                                             BUFFER_OFFSET(e.base_index * the_mesh->geometry()->index_size()),
                                             e.base_vertex);
#else
                    glDrawElements(primitive_type,
                                   e.num_indices,
                                   the_mesh->geometry()->index_type(),
                                   BUFFER_OFFSET(e.base_index * the_mesh->geometry()->index_size()));
#endif
                    KINSKI_CHECK_GL_ERRORS();
                }
            }
        }
        else
        {
            glDrawElements(the_mesh->geometry()->primitive_type(),
                           the_mesh->geometry()->indices().size(), the_mesh->geometry()->index_type(),
                           BUFFER_OFFSET(0));
            KINSKI_CHECK_GL_ERRORS();
        }
    }
    else
    {
        glDrawArrays(the_mesh->geometry()->primitive_type(), 0,
                     the_mesh->geometry()->vertices().size());
        KINSKI_CHECK_GL_ERRORS();
    }

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
        float scale = theLight->radius();

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
            
            default:
                LOG_WARNING << "light type not handled";
                return;
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

    void draw_boundingbox(const gl::AABB& the_aabb)
    {
        static MeshPtr line_mesh;
        if(!line_mesh){ line_mesh = gl::Mesh::create(gl::Geometry::create_box_lines()); }
        glm::mat4 center_mat = glm::translate(glm::mat4(), the_aabb.center());
        glm::mat4 scale_mat = glm::scale(glm::mat4(), vec3(the_aabb.width(),
                                                           the_aabb.height(),
                                                           the_aabb.depth()));
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
            MeshConstPtr m = the_mesh.lock();
            if(m->geometry()->normals().empty()) return;
            GeometryPtr geom = Geometry::create();
            geom->set_primitive_type(GL_LINES);
            gl::MaterialPtr mat = gl::Material::create();
            MeshPtr line_mesh = gl::Mesh::create(geom, mat);
            vector<vec3> &thePoints = geom->vertices();
            vector<vec4> &theColors = geom->colors();
            const vector<vec3> &vertices = m->geometry()->vertices();
            const vector<vec3> &normals = m->geometry()->normals();

            float length = (m->geometry()->aabb().max -
                            m->geometry()->aabb().min).length() * 5;

            for (uint32_t i = 0; i < vertices.size(); i++)
            {
                thePoints.push_back(vertices[i]);
                thePoints.push_back(vertices[i] + normals[i] * length);
                theColors.push_back(colorGrey);
                theColors.push_back(colorRed);
            }
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
                     uint32_t the_num_segments)
    {
        static gl::MaterialPtr color_mat;

        if(!color_mat)
        {
            color_mat = gl::Material::create();
            color_mat->set_depth_test(false);
            color_mat->set_depth_write(false);
        }
        color_mat->set_diffuse(the_color);
        draw_circle(center, radius, color_mat, solid, the_num_segments);
    }

///////////////////////////////////////////////////////////////////////////////

    void draw_circle(const glm::vec2 &center, float the_radius, const MaterialPtr &theMaterial,
                     bool solid, uint32_t the_num_segments)
    {
        constexpr uint32_t max_num_buffer_meshes = 10;
        static std::unordered_map<uint32_t, gl::MeshPtr> solid_meshes, line_meshes;
        static gl::MaterialPtr default_mat;

        // automatically determine the number of segments from the circumference
        if(!the_num_segments){ the_num_segments = (int)floor(the_radius * M_PI * 2);}
        the_num_segments = std::max(the_num_segments, 2U);

        gl::MeshPtr our_mesh;
        auto it = solid ? solid_meshes.find(the_num_segments) : line_meshes.find(the_num_segments);
        if(it != (solid ? solid_meshes.end() : line_meshes.end())){ our_mesh = it->second; }

        if(!our_mesh)
        {
            GeometryPtr geom = solid ? Geometry::create_solid_circle(the_num_segments) :
                Geometry::create_circle(the_num_segments);
            default_mat = gl::Material::create();
            default_mat->set_depth_test(false);
            default_mat->set_depth_write(false);
            our_mesh = gl::Mesh::create(geom, default_mat);

            // cleanup
            if(solid_meshes.size() >= max_num_buffer_meshes){ solid_meshes.clear(); }
            if(line_meshes.size() >= max_num_buffer_meshes){ line_meshes.clear(); }

            // add our newly created mesh
            if(solid){ solid_meshes[the_num_segments] = our_mesh; }
            else{ line_meshes[the_num_segments] = our_mesh; }
        }
        our_mesh->material() = theMaterial ? theMaterial : default_mat;
        mat4 projectionMatrix = ortho(0.0f, g_viewport_dim[0], 0.0f, g_viewport_dim[1], 0.0f, 1.0f);
        mat4 modelView = glm::scale(mat4(), vec3(the_radius));
        modelView[3] = vec4(center.x, g_viewport_dim[1] - center.y, 0, modelView[3].w);

        ScopedMatrixPush m(MODEL_VIEW_MATRIX), p(PROJECTION_MATRIX);
        load_matrix(PROJECTION_MATRIX, projectionMatrix);
        load_matrix(MODEL_VIEW_MATRIX, modelView);
        draw_mesh(our_mesh);
    }

///////////////////////////////////////////////////////////////////////////////

    KINSKI_API gl::Texture render_to_texture(const gl::SceneConstPtr &theScene,
                                             const FboPtr &the_fbo,
                                             const gl::CameraPtr &theCam)
    {
        if(!the_fbo)
        {
            LOG_WARNING << "trying to use an uninitialized FBO";
            return gl::Texture();
        }

        // push framebuffer and viewport states
        gl::SaveViewPort sv; gl::SaveFramebufferBinding sfb;
        gl::set_window_dimension(the_fbo->size());
        the_fbo->bind();
        gl::clear();
        theScene->render(theCam);
        return the_fbo->texture();
    }

    KINSKI_API gl::Texture render_to_texture(const FboPtr &the_fbo, std::function<void()> the_functor)
    {
        if(!the_fbo)
        {
            LOG_WARNING << "trying to use an uninitialized FBO";
            return gl::Texture();
        }
        // push framebuffer and viewport states
        gl::SaveViewPort sv; gl::SaveFramebufferBinding sfb;
        gl::set_window_dimension(the_fbo->size());
        the_fbo->bind();
        the_functor();
        return the_fbo->texture();
    }

///////////////////////////////////////////////////////////////////////////////

    void apply_material(const MaterialPtr &the_mat, bool force_apply, const ShaderPtr &override_shader,
                        std::map<ImagePtr, gl::Texture> *the_img_tex_cache)
    {
        KINSKI_CHECK_GL_ERRORS();
        static MaterialPtr last_mat = gl::Material::create();
        if(!the_mat) return;

        // process texture queue
        auto it = the_mat->queued_textures().begin(), end = the_mat->queued_textures().end();

        for(;it != end;)
        {
            auto &pair = *it;

            constexpr float anisotropic_lvl = 8.f;

            // no DXT compression for normal maps
            bool use_compression = pair.second.key != (uint32_t)Texture::Usage::NORMAL;

            if(pair.second.status == gl::Material::AssetLoadStatus::NOT_LOADED)
            {
                try
                {
                    auto t = gl::create_texture_from_file(pair.first, true, use_compression, anisotropic_lvl);
                    the_mat->add_texture(t, pair.second.key);

                    // copy iterator before incrementing
                    auto delete_it = it++;

                    // remove last iterator from map
                    the_mat->queued_textures().erase(delete_it);
                }
                catch(Exception &e)
                {
                    LOG_WARNING << e.what();
                    pair.second.status = gl::Material::AssetLoadStatus::NOT_FOUND;
                    it++;
                }
            }
            else if(pair.second.status == gl::Material::AssetLoadStatus::IMAGE_LOADED)
            {
                gl::Texture t;

                if(the_img_tex_cache)
                {
                    auto img_tex_it = the_img_tex_cache->find(pair.second.image);
                    if(img_tex_it != the_img_tex_cache->end())
                    {
                        t = img_tex_it->second;
                        LOG_TRACE << "using cached texture: " << pair.first;
                    }
                }

                if(!t)
                {
                    LOG_TRACE << "creating texture: " << pair.first;
                    t = gl::create_texture_from_image(pair.second.image, true, use_compression, anisotropic_lvl);
                    if(the_img_tex_cache){ (*the_img_tex_cache)[pair.second.image] = t; }
                }
                the_mat->add_texture(t, pair.second.key);

                // copy iterator before incrementing
                auto delete_it = it++;

                // remove last iterator from map
                the_mat->queued_textures().erase(delete_it);
            }
        }

        // shader queue
        if(the_mat->queued_shader() != gl::ShaderType::NONE)
        {
            try{ the_mat->set_shader(gl::create_shader(the_mat->queued_shader())); }
            catch(Exception &e){ LOG_WARNING << e.what(); }

        }
        if(!the_mat->shader()){ the_mat->set_shader(gl::create_shader(gl::ShaderType::UNLIT)); }

        // bind the shader
        gl::ShaderPtr shader = override_shader ? override_shader : the_mat->shader();
        shader->bind();
        KINSKI_CHECK_GL_ERRORS();

        // twoSided
        if(force_apply || last_mat->culling() != the_mat->culling() || last_mat->wireframe() != the_mat->wireframe())
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
        if(force_apply || last_mat->wireframe() != the_mat->wireframe())
            glPolygonMode(GL_FRONT_AND_BACK, the_mat->wireframe() ? GL_LINE : GL_FILL);
#endif

        KINSKI_CHECK_GL_ERRORS();

        // read write depth buffer ?
        if(force_apply || last_mat->depth_test() != the_mat->depth_test())
        {
            if(the_mat->depth_test()) { glEnable(GL_DEPTH_TEST); }
            else { glDisable(GL_DEPTH_TEST); }
        }
        KINSKI_CHECK_GL_ERRORS();

        if(force_apply || last_mat->depth_write() != the_mat->depth_write())
        {
            if(the_mat->depth_write()){ glDepthMask(GL_TRUE); }
            else{ glDepthMask(GL_FALSE); }
        }
        KINSKI_CHECK_GL_ERRORS();

        if(force_apply || last_mat->stencil_test() != the_mat->stencil_test())
        {
            if(the_mat->stencil_test()){ glEnable(GL_STENCIL_TEST); }
            else{ glDisable(GL_STENCIL_TEST); }

        }
        KINSKI_CHECK_GL_ERRORS();

        // scissor test
        if(force_apply || last_mat->scissor_rect() != the_mat->scissor_rect())
        {
            auto rect = the_mat->scissor_rect();

            if(the_mat->scissor_rect() == Area_<uint32_t>()){ glDisable(GL_SCISSOR_TEST); }
            else
            {
                glEnable(GL_SCISSOR_TEST);
                glScissor(rect.x0, gl::window_dimension().y - rect.y1, rect.width(), rect.height());
            }
        }
        KINSKI_CHECK_GL_ERRORS();

        if(force_apply || last_mat->blending() != the_mat->blending())
        {
            if(!the_mat->blending()){ glDisable(GL_BLEND); }
            else{ glEnable(GL_BLEND); }
        }
        KINSKI_CHECK_GL_ERRORS();
        
        if(force_apply || last_mat->blend_factors() != the_mat->blend_factors())
        {
            glBlendFunc(the_mat->blend_src(), the_mat->blend_dst());
        }
        KINSKI_CHECK_GL_ERRORS();

#if !defined(KINSKI_GLES_2)
        if(force_apply || last_mat->blend_equation() != the_mat->blend_equation())
        {
            glBlendEquation(the_mat->blend_equation());
        }
        KINSKI_CHECK_GL_ERRORS();
#endif

        if(force_apply || last_mat->point_size() != the_mat->point_size())
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
        if(force_apply || last_mat->line_width() != the_mat->line_width())
        {
            glLineWidth(the_mat->line_width());
            KINSKI_CHECK_GL_ERRORS();
        }
#endif

        // add texturemaps
        int32_t tex_unit = 0, tex_2d = 0, num_textures = 0;
#if !defined(KINSKI_GLES_2)
        int32_t tex_3d = 0, tex_2d_array = 0, tex_cube = 0;
#endif
        
#if !defined(KINSKI_GLES)
        int32_t tex_rect = 0;
#endif
        char buf[512];

        for(const auto &pair : the_mat->textures())
        {
            auto t = pair.second;
            if(!t){ continue; }
            
            num_textures++;
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
            the_mat->uniform(buf, tex_unit++);
        }
        shader->uniform("u_numTextures", num_textures);

        KINSKI_CHECK_GL_ERRORS();

        // update uniform buffers and uniform values for current shader
        the_mat->update_uniforms(shader);

        *last_mat = *the_mat;
    }

///////////////////////////////////////////////////////////////////////////////

    void reset_state()
    {
        static auto mat = gl::Material::create();
        gl::apply_material(mat, true);
#if !defined(KINSKI_GLES_2)
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
#endif
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

        auto aabb = m->aabb();

        // checks if p is inside the aabb of our mesh)
        if(!aabb.intersect(p)) return false;

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

        float ray_offset = 2 * glm::length(src->aabb().transform(src->global_transform()).halfExtents());
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

                case ShaderType::UNLIT_PANORAMA:
                    vert_src = unlit_cube_vert;
                    frag_src = unlit_panorama_frag;
                    break;
                    
                case ShaderType::GOURAUD:
                    vert_src = gouraud_vert;
                    frag_src = gouraud_frag;
                    break;

                case ShaderType::GOURAUD_SKIN:
                    vert_src = gouraud_skin_vert;
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
                
                case ShaderType::UNLIT_CUBE:
                    vert_src = unlit_cube_vert;
                    frag_src = unlit_cube_frag;
                    break;

                case ShaderType::UNLIT_DISPLACE:
                    vert_src = unlit_displace_vert;
                    frag_src = unlit_frag;
                    break;

                case ShaderType::RESOLVE:
                    vert_src = unlit_vert;
                    frag_src = resolve_frag;
                    break;
                    
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
        return g_shader_names[ShaderType::NONE];
    }

///////////////////////////////////////////////////////////////////////////////

void get_texture_format(int the_num_comps, bool compress, GLenum the_datatype, GLenum *out_format,
                        GLenum *out_internal_format)
{
    compress = compress && (the_datatype == GL_UNSIGNED_BYTE);

    switch(the_num_comps)
    {
#ifdef KINSKI_GLES
        case 1:
            *out_internal_format = *out_format = GL_LUMINANCE;
            break;
        case 2:
            *out_internal_format = *out_format = GL_LUMINANCE_ALPHA;
            break;
        case 3:
            *out_internal_format = *out_format = GL_RGB;
            // needs precompressed image and call to glCompressedTexImage2D
            //                internal_format = compress ? GL_ETC1_RGB8_OES : GL_RGB;
            break;
        case 4:
            *out_internal_format = *out_format = GL_RGBA;
        default:
            break;
#else
        case 1:
            *out_format = GL_RED;
            if(compress){ *out_internal_format = GL_COMPRESSED_RED_RGTC1; }
            else if(the_datatype == GL_FLOAT){ *out_internal_format = GL_R32F; }
            else if(the_datatype == GL_UNSIGNED_BYTE){ *out_internal_format = GL_R8; }
            else if(the_datatype == GL_UNSIGNED_SHORT){ *out_internal_format = GL_R16; }
            break;
        case 2:
            *out_format = GL_RG;
            if(compress){ *out_internal_format = GL_COMPRESSED_RG_RGTC2; }
            else if(the_datatype == GL_FLOAT){ *out_internal_format = GL_RG32F; }
            else if(the_datatype == GL_UNSIGNED_BYTE){ *out_internal_format = GL_RG8; }
            else if(the_datatype == GL_UNSIGNED_SHORT){ *out_internal_format = GL_RG16; }
            break;
        case 3:
            *out_format = GL_RGB;
            if(compress){ *out_internal_format = GL_COMPRESSED_RGB_S3TC_DXT1_EXT; }
            else if(the_datatype == GL_FLOAT){ *out_internal_format = GL_RGB32F; }
            else if(the_datatype == GL_UNSIGNED_BYTE){ *out_internal_format = GL_RGB8; }
            else if(the_datatype == GL_UNSIGNED_SHORT){ *out_internal_format = GL_RGB16; }
            break;
        case 4:
            *out_format = GL_RGBA;
            if(compress){ *out_internal_format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; }
            else if(the_datatype == GL_FLOAT){ *out_internal_format = GL_RGBA32F; }
            else if(the_datatype == GL_UNSIGNED_BYTE){ *out_internal_format = GL_RGBA8; }
            else if(the_datatype == GL_UNSIGNED_SHORT){ *out_internal_format = GL_RGBA16; }
        default:
            break;
#endif
    }
}

///////////////////////////////////////////////////////////////////////////////

Texture create_texture_from_image(const ImagePtr& the_img, bool mipmap,
                                  bool compress, GLfloat anisotropic_filter_lvl)
{
    Texture ret;
    if(!the_img){ return ret; }
    bool use_float = (the_img->num_bytes() / (the_img->width() * the_img->height() * the_img->num_components())) > 1;
    LOG_TRACE_IF(use_float) << "creating FLOAT texture ...";
    auto data_type = use_float ? GL_FLOAT : GL_UNSIGNED_BYTE;

    GLenum format = 0, internal_format = 0;
    get_texture_format(the_img->num_components(), compress, data_type, &format, &internal_format);

    Texture::Format fmt;
    fmt.internal_format = internal_format;
    fmt.datatype = data_type;

    if(mipmap)
    {
        fmt.mipmapping = true;
        fmt.min_filter = GL_LINEAR_MIPMAP_NEAREST;
    }
    void *data = the_img->data();

#if !defined(KINSKI_GLES)
    gl::Buffer pixel_buf;
    pixel_buf.set_data(the_img->data(), the_img->num_bytes());
    pixel_buf.bind(GL_PIXEL_UNPACK_BUFFER);
    data = nullptr;
#endif
    ret = Texture(data, format, the_img->width(), the_img->height(), fmt);
    ret.set_flipped();
    KINSKI_CHECK_GL_ERRORS();

#if !defined(KINSKI_GLES)
    Image::Type bgr_types[] = {Image::Type::BGR, Image::Type::BGRA};
    if(contains(bgr_types, the_img->type())){ ret.set_swizzle(GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA); }
#endif

    ret.set_anisotropic_filter(anisotropic_filter_lvl);
    return ret;
}

///////////////////////////////////////////////////////////////////////////////

Texture create_texture_from_data(const std::vector<uint8_t> &the_data, bool mipmap,
                                 bool compress, GLfloat anisotropic_filter_lvl)
{
    ImagePtr img;
    Texture ret;
    try {img = create_image_from_data(the_data);}
    catch (ImageLoadException &e)
    {
        LOG_ERROR << e.what();
        return ret;
    }
    ret = create_texture_from_image(img, mipmap, compress, anisotropic_filter_lvl);
    return ret;
}

///////////////////////////////////////////////////////////////////////////////

Texture create_texture_from_file(const std::string &theFileName, bool mipmap, bool compress,
                                 GLfloat anisotropic_filter_lvl)
{
    ImagePtr img = create_image_from_file(theFileName);
    Texture ret = create_texture_from_image(img, mipmap, compress, anisotropic_filter_lvl);
    return ret;
}

///////////////////////////////////////////////////////////////////////////////

ImagePtr create_image_from_texture(const gl::Texture &the_texture)
{
#if !defined(KINSKI_GLES)
    if(the_texture)
    {

        auto ret = Image_<uint8_t >::create(the_texture.width(), the_texture.height(), 4);
        the_texture.bind();
        glGetTexImage(the_texture.target(), 0, GL_RGBA, GL_UNSIGNED_BYTE, ret->data());
        ret->flip();
        ret->m_type = Image::Type::RGBA;
        return ret;
    }
#endif
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////

Texture create_cube_texture_from_file(const std::string &the_path, CubeTextureLayout the_layout,
                                      bool compress)
{
//    // load image
//    auto img = create_image_from_file(the_path);
//
//    if(img && the_layout == CubeTextureLayout::H_CROSS)
//    {
//        uint32_t sub_w = img->width / 4, sub_h = img->height / 3;
//
//        // TODO: take layout into account
//        // create 6 images
//        std::vector<ImagePtr> out_images(6);
//        gl::ivec2 offsets[6] =
//                {
//                        gl::ivec2(2, 1),
//                        gl::ivec2(0, 1),
//                        gl::ivec2(1, 0),
//                        gl::ivec2(1, 2),
//                        gl::ivec2(3, 1),
//                        gl::ivec2(1, 1),
//                };
//
//        for(int i = 0; i < 6; i++)
//        {
//            img->roi.x0 = offsets[i].x * sub_w;
//            img->roi.y0 = offsets[i].y * sub_h;
//            img->roi.x1 = img->roi.x0 + sub_w;
//            img->roi.y1 = img->roi.y0 + sub_h;
//            out_images[i] = Image::create(sub_w, sub_h, img->num_components());
//            copy_image(img, out_images[i]);
////            out_images[i]->flip(true);
//        }
//        return create_cube_texture_from_images(out_images, compress);
//    }
    return Texture();
}

///////////////////////////////////////////////////////////////////////////////

gl::Texture create_cube_texture_from_images(const std::vector<ImagePtr> &the_planes, bool mipmap, bool compress)
{
    gl::Texture ret;
#if !defined(KINSKI_GLES)
    // check if number and sizes of input textures match
    if(the_planes.size() != 6 || !the_planes[0])
    {
        LOG_WARNING << "cube map creation failed. number of input textures must be 6 -- "
                    << the_planes.size() << " provided";
        return ret;
    }

    uint32_t width = the_planes[0]->width(), height = the_planes[0]->height();
    uint32_t num_components = the_planes[0]->num_components();

    for(auto img : the_planes)
    {
        if(!img || img->width() != width || img->height() != height ||
           img->num_components() != num_components)
        {
            LOG_WARNING << "cube map creation failed. size/type of input textures not consistent";
            return ret;
        }
    }

    // create and bind cube map
    GLenum format = 0, internal_format = 0;
    get_texture_format(num_components, compress, GL_UNSIGNED_BYTE, &format, &internal_format);

    gl::Texture::Format fmt;
    fmt.target = GL_TEXTURE_CUBE_MAP;
    fmt.internal_format = internal_format;
    ret = gl::Texture(nullptr, format, width, height, fmt);
    ret.bind();

    // create PBO
    gl::Buffer pixel_buf = gl::Buffer(GL_PIXEL_UNPACK_BUFFER, GL_STATIC_COPY);

    for(uint32_t i = 0; i < 6; i++)
    {
        // copy data to PBO
        pixel_buf.set_data(the_planes[i]->data(), the_planes[i]->num_bytes());

        // bind PBO
        pixel_buf.bind(GL_PIXEL_UNPACK_BUFFER);

        // copy data from PBO to appropriate image plane
        glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, width, height, format,
                        GL_UNSIGNED_BYTE, nullptr);
    }
#endif
    ret.set_mipmapping(mipmap);
    return ret;
}

/////////////////////////////////////////////////////////////////////////////////

gl::Texture create_cube_texture_from_panorama(const gl::Texture &the_panorama, size_t the_size, bool mipmap,
                                              bool compress)
{
#if !defined(KINSKI_GLES_2)
    if(!the_panorama || the_panorama.target() != GL_TEXTURE_2D)
    {
        LOG_WARNING << "could not convert panorma to cubemap";
        return gl::Texture();
    }
    LOG_DEBUG << "creating cubemap from panorama ...";
    auto mat = gl::Material::create();
    mat->set_culling(gl::Material::CULL_FRONT);
    mat->add_texture(the_panorama, Texture::Usage::ENVIROMENT);
    mat->set_depth_test(false);
    mat->set_depth_write(false);
    auto box_mesh = gl::Mesh::create(gl::Geometry::create_box(gl::vec3(.5f)), mat);

    // render enviroment cubemap here
    auto data_type = the_panorama.datatype();
    auto cube_cam = gl::CubeCamera::create(.1f, 10.f);
    gl::Texture::Format color_fmt;
    color_fmt.datatype = data_type;
    color_fmt.internal_format = the_panorama.internal_format();
    auto cube_fbo = gl::create_cube_framebuffer(the_size, true, data_type);
    auto cube_shader = gl::Shader::create(cube_vert, unlit_panorama_frag, cube_layers_env_geom);

    auto cam_matrices = cube_cam->view_matrices();

    cube_shader->bind();
    cube_shader->uniform("u_view_matrix", cam_matrices);
    cube_shader->uniform("u_projection_matrix", cube_cam->projection_matrix());

    auto cube_tex = gl::render_to_texture(cube_fbo, [box_mesh, cube_shader]()
    {
        gl::draw_mesh(box_mesh, cube_shader);
    });
    KINSKI_CHECK_GL_ERRORS();

    // make a copy
    if(mipmap || compress)
    {
        GLenum format = 0, internal_format = 0;
        get_texture_format(3, compress, data_type, &format, &internal_format);

        gl::Texture::Format fmt;
        fmt.target = GL_TEXTURE_CUBE_MAP;
        fmt.internal_format = internal_format;
        fmt.datatype = data_type;
        auto ret = gl::Texture(nullptr, format, the_size, the_size, fmt);


        // create PBO
        gl::Buffer pixel_buf = gl::Buffer(GL_PIXEL_PACK_BUFFER, GL_STATIC_COPY);
        pixel_buf.set_data(nullptr, the_size * the_size * 4 * (data_type == GL_FLOAT ? 4 : 1));

        // bind PBO for reading and writing
        pixel_buf.bind(GL_PIXEL_PACK_BUFFER);
        pixel_buf.bind(GL_PIXEL_UNPACK_BUFFER);

        for(uint32_t i = 0; i < 6; i++)
        {
            // copy data to PBO
            cube_tex.bind();
            glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, data_type, nullptr);

            // copy data from PBO to appropriate image plane
            ret.bind();
            glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, the_size, the_size, format,
                            data_type, nullptr);
        }
        ret.set_mipmapping(mipmap);
        KINSKI_CHECK_GL_ERRORS();
        return ret;
    }
    return cube_tex;
#endif
    return Texture();
}

}}//namespace
