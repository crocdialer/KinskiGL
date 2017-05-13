//
// Created by Fabian on 02.05.17.
//
#pragma once

#include "SceneRenderer.hpp"

namespace kinski{ namespace gl {

DEFINE_CLASS_PTR(DeferredRenderer);

class KINSKI_API DeferredRenderer : public SceneRenderer
{
public:

    enum G_BUFFER
    {
        G_BUFFER_ALBEDO = 0,
        G_BUFFER_NORMAL = 1,
        G_BUFFER_POSITION = 2,
        G_BUFFER_SPECULAR = 3,
        G_BUFFER_SIZE = 4
    };

    static DeferredRendererPtr create();
    uint32_t render_scene(const gl::SceneConstPtr &the_scene, const CameraPtr &the_cam,
                          const std::set<std::string> &the_tags = {}) override;


    gl::Fbo g_buffer(){ return m_geometry_fbo; }

    gl::Texture final_texture();
private:

    DeferredRenderer();
    void init();
    void geometry_pass(const gl::vec2 &the_size, const RenderBinPtr &the_renderbin);
    void light_pass(const gl::vec2 &the_size, const RenderBinPtr &the_renderbin);
    void stencil_pass(const RenderBinPtr &the_renderbin);
    void render_light_volumes(const RenderBinPtr &the_renderbin, bool stencil_pass);
    gl::Fbo shadow_pass(const RenderBinPtr &the_renderbin, const gl::LightPtr &l);

    gl::ShaderPtr m_shader_g_buffer, m_shader_g_buffer_skin, m_shader_g_buffer_normalmap;
    gl::ShaderPtr m_shader_shadow, m_shader_shadow_skin;
    gl::Fbo m_geometry_fbo, m_lighting_fbo;

    gl::MaterialPtr m_mat_lighting, m_mat_lighting_shadow, m_mat_stencil;
    gl::MeshPtr m_mesh_sphere, m_mesh_cone, m_frustum_mesh;
};

}}
