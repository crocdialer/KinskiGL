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

DeferredRenderer::DeferredRenderer()
{

}

uint32_t DeferredRenderer::render_scene(const gl::SceneConstPtr &the_scene, const CameraPtr &the_cam,
                                        const std::set<std::string> &the_tags)
{
    // culling
    auto render_bin = cull(the_scene, the_cam, the_tags);

    // create G-buffer, if necessary, and fill it
    geometry_pass(gl::window_dimension(), render_bin);

    // lighting pass
    light_pass(gl::window_dimension(), render_bin);

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
        fmt.set_num_color_buffers(G_BUFFER_SIZE);
        m_geometry_fbo = gl::Fbo(the_size, fmt);
        KINSKI_CHECK_GL_ERRORS();
    }

    std::list<RenderBin::item> opaque_items, blended_items;
    sort_render_bin(the_renderbin, opaque_items, blended_items);

    // bind G-Buffer
    gl::SaveViewPort sv; gl::SaveFramebufferBinding sfb;
    gl::set_window_dimension(m_geometry_fbo.size());
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
    if(!m_lighting_fbo || m_lighting_fbo.size() != the_size)
    {
        gl::Fbo::Format fmt;
        fmt.enable_stencil_buffer(true);
        m_lighting_fbo = gl::Fbo(the_size, fmt);
        m_lighting_fbo.set_depth_texture(m_geometry_fbo.depth_texture());
        KINSKI_CHECK_GL_ERRORS();
    }

    auto mat = gl::Material::create();
    mat->set_depth_write(false);
    mat->set_depth_test(false);
    mat->set_two_sided();
    mat->set_blending(true);
    mat->set_blend_equation(GL_FUNC_ADD);
    mat->set_blend_factors(GL_ONE, GL_ONE);
    mat->textures().clear();
    for(uint32_t i = 0; i < G_BUFFER_SIZE; ++i){ mat->add_texture(m_geometry_fbo.texture(i)); }

    gl::MeshPtr sphere = gl::Mesh::create(gl::Geometry::create_sphere(1.f, 32), mat);

    gl::ScopedMatrixPush sp(gl::MODEL_VIEW_MATRIX);
    gl::load_matrix(gl::MODEL_VIEW_MATRIX, the_renderbin->camera->view_matrix());

    for(auto l : the_renderbin->lights)
    {
        float d = l.light->max_distance();
        gl::ScopedMatrixPush p(gl::MODEL_VIEW_MATRIX);
        gl::mult_matrix(gl::MODEL_VIEW_MATRIX, glm::scale(l.transform, glm::vec3(d)));
//        gl::draw_mesh(sphere);
    }
}


}}
