//
// Created by Fabian on 02.05.17.
//

#include "Mesh.hpp"
#include "Camera.hpp"
#include "Light.hpp"
#include "Scene.hpp"
#include "ShaderLibrary.h"
#include "DeferredRenderer.hpp"

namespace kinski{ namespace gl{

DeferredRendererPtr DeferredRenderer::create()
{
    return DeferredRendererPtr(new DeferredRenderer());
}

DeferredRenderer::DeferredRenderer(): SceneRenderer()
{

}

void DeferredRenderer::init()
{
#if !defined(KINSKI_GLES)
    auto shader = gl::Shader::create(unlit_vert, deferred_lighting_frag);
    m_mat_lighting = gl::Material::create(shader);
    m_mat_lighting->set_depth_write(false);
    m_mat_lighting->set_stencil_test(true);
    m_mat_lighting->set_depth_test(false);
    m_mat_lighting->set_culling(Material::CULL_FRONT);
    m_mat_lighting->set_blending(true);
    m_mat_lighting->set_blend_equation(GL_FUNC_ADD);
    m_mat_lighting->set_blend_factors(GL_ONE, GL_ONE);
    m_mat_lighting_shadow = gl::Material::create();
    *m_mat_lighting_shadow = *m_mat_lighting;
    m_mat_lighting_shadow->set_shader(gl::Shader::create(unlit_vert, deferred_lighting_shadow_frag));

    m_mat_stencil = gl::Material::create(shader);
    m_mat_stencil->set_depth_test(true);
    m_mat_stencil->set_depth_write(false);
    m_mat_stencil->set_stencil_test(true);
    m_mat_stencil->set_culling(Material::CULL_NONE);

    m_mesh_sphere = gl::Mesh::create(gl::Geometry::create_sphere(1.f, 32), m_mat_lighting);
    m_mesh_cone = gl::Mesh::create(gl::Geometry::create_cone(1.f, 1.f, 24), m_mat_lighting);

    glm::mat4 rot_spot_mat = glm::rotate(glm::mat4(), glm::half_pi<float>(), gl::X_AXIS);

    for(auto &vert : m_mesh_cone->geometry()->vertices())
    {
        vert -= vec3(0, 1, 0);
        vert = (rot_spot_mat * glm::vec4(vert, 1.f)).xyz();
    }

    set_shadowmap_size(glm::vec2(1024));
#endif
}

uint32_t DeferredRenderer::render_scene(const gl::SceneConstPtr &the_scene, const CameraPtr &the_cam,
                                        const std::set<std::string> &the_tags)
{
#if !defined(KINSKI_GLES)

    // culling
    auto render_bin = cull(the_scene, the_cam, the_tags);

    {
        gl::SaveFramebufferBinding sfb;

        // create G-buffer, if necessary, and fill it
        geometry_pass(gl::window_dimension(), render_bin);

        // lighting pass
        light_pass(gl::window_dimension(), render_bin);
    }
    Area_<int> src(0, 0, m_lighting_fbo.size().x - 1, m_lighting_fbo.size().y - 1);
    Area_<int> dst(0, 0, gl::window_dimension().x - 1, gl::window_dimension().y - 1);
//    m_lighting_fbo.blit_to_current(src, dst, GL_NEAREST, GL_COLOR_BUFFER_BIT);
    glDepthMask(GL_TRUE);
    m_geometry_fbo.blit_to_current(src, dst, GL_NEAREST, GL_DEPTH_BUFFER_BIT);

    // skybox drawing
    if(the_scene->skybox())
    {
        gl::ScopedMatrixPush mv(gl::MODEL_VIEW_MATRIX), proj(gl::PROJECTION_MATRIX);
        gl::set_projection(the_cam);
        mat4 m = the_cam->view_matrix();
        m[3] = vec4(0, 0, 0, 1);
        gl::load_matrix(gl::MODEL_VIEW_MATRIX, m);
        gl::draw_mesh(the_scene->skybox());
    }
    gl::draw_texture(m_lighting_fbo.texture(), gl::window_dimension());

    // return number of rendered objects
    return render_bin->items.size();
#endif
    return 0;
}

void DeferredRenderer::geometry_pass(const gl::vec2 &the_size, const RenderBinPtr &the_renderbin)
{
#if !defined(KINSKI_GLES)

    if(!m_geometry_fbo || m_geometry_fbo.size() != the_size)
    {
        gl::Fbo::Format fmt;
        fmt.set_color_internal_format(GL_RGB32F);
        fmt.enable_stencil_buffer(true);
//        fmt.set_num_samples(4);
        fmt.set_num_color_buffers(G_BUFFER_SIZE - 1);
        m_geometry_fbo = gl::Fbo(the_size, fmt);

        gl::Texture::Format tex_fmt;
        tex_fmt.set_internal_format(GL_RGBA32F);
        tex_fmt.set_data_type(GL_FLOAT);
        gl::Texture t(the_size.x, the_size.y, tex_fmt);
        m_geometry_fbo.add_attachment(t);

        for(uint32_t i = 0; i < G_BUFFER_SIZE; ++i)
        {
            m_geometry_fbo.texture(i).set_mag_filter(GL_NEAREST);
            m_geometry_fbo.texture(i).set_min_filter(GL_NEAREST);
        }
        KINSKI_CHECK_GL_ERRORS();
    }

    std::list<RenderBin::item> opaque_items, blended_items;
    sort_render_bin(the_renderbin, opaque_items, blended_items);

    // bind G-Buffer
    m_geometry_fbo.bind();
    gl::clear();
    KINSKI_CHECK_GL_ERRORS();

    // create our shaders
    if(!m_shader_g_buffer){ m_shader_g_buffer = gl::Shader::create(phong_vert, create_g_buffer_frag); }
    if(!m_shader_g_buffer_skin)
    {
        m_shader_g_buffer_skin = gl::Shader::create(phong_skin_vert, create_g_buffer_frag);
    }

    for (const RenderBin::item &item : opaque_items)
    {
        gl::ScopedMatrixPush mv(gl::MODEL_VIEW_MATRIX), proj(gl::PROJECTION_MATRIX);
        gl::set_projection(the_renderbin->camera);
        gl::load_matrix(gl::MODEL_VIEW_MATRIX, item.transform);
        gl::ShaderPtr shader = item.mesh->geometry()->has_bones() ? m_shader_g_buffer_skin : m_shader_g_buffer;
        gl::draw_mesh(item.mesh, shader);
    }
#endif
}

void DeferredRenderer::light_pass(const gl::vec2 &the_size, const RenderBinPtr &the_renderbin)
{
#if !defined(KINSKI_GLES)

    if(!m_lighting_fbo || m_lighting_fbo.size() != m_geometry_fbo.size())
    {
        gl::Fbo::Format fmt;
        fmt.enable_stencil_buffer(true);
        m_lighting_fbo = gl::Fbo(m_geometry_fbo.size(), fmt);
        m_lighting_fbo.set_depth_texture(m_geometry_fbo.depth_texture());
        KINSKI_CHECK_GL_ERRORS();

        if(!m_mat_lighting){ init(); }
        m_mat_lighting->uniform("u_window_dimension", gl::window_dimension());
        m_mat_lighting_shadow->uniform("u_window_dimension", gl::window_dimension());
        m_mat_lighting->textures().clear();
        m_mat_lighting_shadow->textures().clear();

        for(uint32_t i = 0; i < G_BUFFER_SIZE; ++i)
        {
            m_mat_lighting->add_texture(m_geometry_fbo.texture(i));
            m_mat_lighting_shadow->add_texture(m_geometry_fbo.texture(i));
        }
        m_mat_lighting_shadow->add_texture(shadow_fbos()[0].depth_texture());
    }
    m_lighting_fbo.bind();

    // update frustum for directional lights
    m_frustum_mesh = gl::create_frustum_mesh(the_renderbin->camera, true);
    auto &verts = m_frustum_mesh->geometry()->vertices();
    for(uint32_t i = 0; i < 4; ++i){ verts[i].z += -.1f; }
    for(uint32_t i = 4; i < 8; ++i){ verts[i].z += 1.f; }

    gl::apply_material(m_mat_lighting);
    update_uniform_buffers(the_renderbin->lights);
    m_mat_lighting->shader()->uniform_block_binding("LightBlock", LIGHT_BLOCK);
    m_mat_lighting_shadow->shader()->uniform_block_binding("LightBlock", LIGHT_BLOCK);

    auto c = gl::COLOR_BLACK;
    c.a = 0.f;
    gl::clear_color(c);
    gl::clear();
    gl::clear_color(gl::COLOR_BLACK);

    // stencil pass
    glStencilFunc(GL_ALWAYS, 0, 0);
    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
    m_lighting_fbo.enable_draw_buffers(false);
    render_light_volumes(the_renderbin, true);

    // light pass
    glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
    m_lighting_fbo.enable_draw_buffers(true);
    render_light_volumes(the_renderbin, false);
#endif
}

gl::Fbo DeferredRenderer::shadow_pass(const RenderBinPtr &the_renderbin, const gl::LightPtr &l)
{
    if(!m_shader_shadow || !m_shader_shadow_skin)
    {
        m_shader_shadow = gl::create_shader(gl::ShaderType::UNLIT);
        m_shader_shadow_skin = gl::create_shader(gl::ShaderType::UNLIT_SKIN);
    }
    if(l->cast_shadow())
    {
        shadow_cams()[0] = gl::create_shadow_camera(l, std::min(1000.f, l->max_distance()));

        auto bin = cull(the_renderbin->scene, shadow_cams()[0]);
        std::list<RenderBin::item> opaque_items, blended_items;
        sort_render_bin(bin, opaque_items, blended_items);

        // offscreen render shadow map here
        gl::render_to_texture(shadow_fbos()[0], [&]()
        {
            gl::reset_state();
            glClear(GL_DEPTH_BUFFER_BIT);
            gl::ScopedMatrixPush mv(gl::MODEL_VIEW_MATRIX), proj(gl::PROJECTION_MATRIX);
            gl::set_projection(bin->camera);

            for(const RenderBin::item &item : opaque_items)
            {
                // filter out non-shadow casters
                if(!(item.mesh->material()->shadow_properties() & gl::Material::SHADOW_CAST)){ continue; }

                gl::load_matrix(gl::MODEL_VIEW_MATRIX, item.transform);
                gl::ShaderPtr shader = item.mesh->geometry()->has_bones() ? m_shader_shadow_skin : m_shader_shadow;
                gl::draw_mesh(item.mesh, shader);
            }

        });
        mat4 shadow_matrix = shadow_cams()[0]->projection_matrix() * shadow_cams()[0]->view_matrix() * the_renderbin->camera->global_transform();
        m_mat_lighting_shadow->uniform("u_shadow_matrix", shadow_matrix);
    }
    return shadow_fbos()[0];
}

void DeferredRenderer::render_light_volumes(const RenderBinPtr &the_renderbin, bool stencil_pass)
{
    gl::ScopedMatrixPush mv(gl::MODEL_VIEW_MATRIX), proj(gl::PROJECTION_MATRIX);
    gl::set_projection(the_renderbin->camera);

    gl::MaterialPtr mat = stencil_pass ? m_mat_stencil : m_mat_lighting;
    m_frustum_mesh->material() = m_mesh_sphere->material() = m_mesh_cone->material() = mat;
    int light_index = 0;

    for(auto l : the_renderbin->lights)
    {
        if(!stencil_pass && l.light->cast_shadow())
        {
            auto shadow_fbo = shadow_pass(the_renderbin, l.light);
            mat = m_mat_lighting_shadow;
            m_frustum_mesh->material() = m_mesh_sphere->material() = m_mesh_cone->material() = mat;
        }
        float d = l.light->max_distance(1.f / 10.f);
        mat->uniform("u_light_index", light_index++);

        switch(l.light->type())
        {
            case Light::DIRECTIONAL:
            {
                gl::load_identity(gl::MODEL_VIEW_MATRIX);
                gl::draw_mesh(m_frustum_mesh);
                break;
            }
            case Light::POINT:
            {
                gl::load_matrix(gl::MODEL_VIEW_MATRIX, glm::scale(l.transform, glm::vec3(d)));
                gl::draw_mesh(m_mesh_sphere);
                break;
            }
            case Light::SPOT:
            {
                float r_scale = tan(glm::radians(l.light->spot_cutoff())) * d;
                gl::load_matrix(gl::MODEL_VIEW_MATRIX, glm::scale(l.transform, vec3(r_scale, r_scale, d)));
                gl::draw_mesh(m_mesh_cone);
                break;
            }
        }
    }
}

gl::Texture DeferredRenderer::final_texture()
{
    return m_lighting_fbo.texture();
}

}}
