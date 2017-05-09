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

DeferredRenderer::DeferredRenderer()
{

}

void DeferredRenderer::init()
{
    auto shader = gl::Shader::create(unlit_vert, deferred_lighting_frag);
    m_mat_lighting = gl::Material::create(shader);
    m_mat_lighting->set_depth_write(false);
    m_mat_lighting->set_stencil_test(true);
    m_mat_lighting->set_depth_test(false);
    m_mat_lighting->set_culling(Material::CULL_FRONT);
    m_mat_lighting->set_blending(true);
    m_mat_lighting->set_blend_equation(GL_FUNC_ADD);
    m_mat_lighting->set_blend_factors(GL_ONE, GL_ONE);

    m_mat_stencil = gl::Material::create(shader);
    m_mat_stencil->set_depth_test(true);
    m_mat_stencil->set_depth_write(false);
    m_mat_stencil->set_stencil_test(true);
    m_mat_stencil->set_culling(Material::CULL_NONE);

    m_mesh_sphere = gl::Mesh::create(gl::Geometry::create_sphere(1.f, 32), m_mat_lighting);
    m_mesh_cone = gl::Mesh::create(gl::Geometry::create_cone(1.f, 1.f, 16), m_mat_lighting);

    glm::mat4 rot_spot_mat = glm::rotate(glm::mat4(), glm::half_pi<float>(), gl::X_AXIS);

    for(auto &vert : m_mesh_cone->geometry()->vertices())
    {
        vert -= vec3(0, 1, 0);
        vert = (rot_spot_mat * glm::vec4(vert, 1.f)).xyz();
    }
}

uint32_t DeferredRenderer::render_scene(const gl::SceneConstPtr &the_scene, const CameraPtr &the_cam,
                                        const std::set<std::string> &the_tags)
{
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
    m_lighting_fbo.blit_to_screen(src, dst);
    gl::reset_state();

    // return number of rendered objects
    return render_bin->items.size();
}

void DeferredRenderer::geometry_pass(const gl::vec2 &the_size, const RenderBinPtr &the_renderbin)
{
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

    gl::ShaderPtr shader = m_shader_g_buffer;

    for (const RenderBin::item &item : opaque_items)
    {
        Mesh *m = item.mesh;

        const glm::mat4 &modelView = item.transform;
        mat4 mvp_matrix = the_renderbin->camera->projection_matrix() * modelView;
        mat3 normal_matrix = glm::inverseTranspose(glm::mat3(modelView));

        for(auto &mat : m->materials())
        {
            mat->uniform("u_modelViewMatrix", modelView);
            mat->uniform("u_modelViewProjectionMatrix", mvp_matrix);
            mat->uniform("u_normalMatrix", normal_matrix);
            if(m->geometry()->has_bones()){ mat->uniform("u_bones", m->bone_matrices()); }
            KINSKI_CHECK_GL_ERRORS();
        }

        if(m->geometry()->has_bones()){ shader = m_shader_g_buffer_skin; }
        GLint block_index = shader->uniform_block_index("MaterialBlock");

        if(block_index >= 0)
        {
            glUniformBlockBinding(shader->handle(), block_index, MATERIAL_BLOCK);
            KINSKI_CHECK_GL_ERRORS();
        }
        gl::apply_material(m->material(), false, shader);
        m->bind_vertex_array();

        KINSKI_CHECK_GL_ERRORS();

        if(m->geometry()->has_indices())
        {
            if(!m->entries().empty())
            {
                for (uint32_t i = 0; i < m->entries().size(); i++)
                {
                    // skip disabled entries
                    if(!m->entries()[i].enabled) continue;

                    uint32_t primitive_type = m->entries()[i].primitive_type;
                    primitive_type = primitive_type ? : m->geometry()->primitive_type();

                    int mat_index = clamp<int>(m->entries()[i].material_index,
                                               0,
                                               m->materials().size() - 1);
                    m->bind_vertex_array(mat_index);
                    apply_material(m->materials()[mat_index], false, shader);
                    KINSKI_CHECK_GL_ERRORS();

                    glDrawElementsBaseVertex(primitive_type,
                                             m->entries()[i].num_indices,
                                             m->geometry()->indexType(),
                                             BUFFER_OFFSET(m->entries()[i].base_index
                                                           * sizeof(m->geometry()->indexType())),
                                             m->entries()[i].base_vertex);
                }
            }
            else
            {
                glDrawElements(m->geometry()->primitive_type(),
                               m->geometry()->indices().size(), m->geometry()->indexType(),
                               BUFFER_OFFSET(0));
            }
        }
        else
        {
            glDrawArrays(m->geometry()->primitive_type(), 0, m->geometry()->vertices().size());
        }
        KINSKI_CHECK_GL_ERRORS();
    }
    GL_SUFFIX(glBindVertexArray)(0);
}

void DeferredRenderer::light_pass(const gl::vec2 &the_size, const RenderBinPtr &the_renderbin)
{
    if(!m_lighting_fbo || m_lighting_fbo.size() != m_geometry_fbo.size())
    {
        gl::Fbo::Format fmt;
        fmt.enable_stencil_buffer(true);
        m_lighting_fbo = gl::Fbo(m_geometry_fbo.size(), fmt);
        m_lighting_fbo.set_depth_texture(m_geometry_fbo.depth_texture());
        KINSKI_CHECK_GL_ERRORS();
    }
    m_lighting_fbo.bind();

    if(!m_mat_lighting){ init(); }

    m_mat_lighting->textures().clear();
    m_mat_lighting->uniform("u_window_dimension", gl::window_dimension());
    for(uint32_t i = 0; i < G_BUFFER_SIZE; ++i){ m_mat_lighting->add_texture(m_geometry_fbo.texture(i)); }

    gl::apply_material(m_mat_lighting);
    update_uniform_buffers(the_renderbin->lights);
    GLint block_index = m_mat_lighting->shader()->uniform_block_index("LightBlock");

    if(block_index >= 0)
    {
        glUniformBlockBinding(m_mat_lighting->shader()->handle(), block_index, LIGHT_BLOCK);
        KINSKI_CHECK_GL_ERRORS();
    }
    gl::clear();

    // stencil pass
    glStencilFunc(GL_ALWAYS, 0, 0);
    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
    m_lighting_fbo.enable_draw_buffers(false);
    render_light_volumes(the_renderbin, m_mat_stencil);

    // light pass
    glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
    m_lighting_fbo.enable_draw_buffers(true);
    render_light_volumes(the_renderbin, m_mat_lighting);
}


void DeferredRenderer::render_light_volumes(const RenderBinPtr &the_renderbin, const gl::MaterialPtr &the_mat)
{
    gl::ScopedMatrixPush mv(gl::MODEL_VIEW_MATRIX), proj(gl::PROJECTION_MATRIX);
    gl::set_projection(the_renderbin->camera);

    m_mesh_sphere->material() = the_mat;
    m_mesh_cone->material() = the_mat;
    int light_index = 0;

    for(auto l : the_renderbin->lights)
    {
        float d = l.light->max_distance(1.f / 10.f);
        the_mat->uniform("u_light_index", light_index++);

        switch(l.light->type())
        {
            case Light::DIRECTIONAL:
            {
                //TODO: find proper stencil-op for this to work
                gl::draw_quad(the_mat, gl::window_dimension());
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
