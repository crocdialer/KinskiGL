//
// Created by Fabian on 02.05.17.
//
#pragma once

#include "SceneRenderer.hpp"

namespace kinski{ namespace gl {

class KINSKI_API DeferredRenderer : public SceneRenderer
{
public:

    enum G_BUFFER
    {
        G_BUFFER_ALBEDO = 0, G_BUFFER_NORMAL = 1, G_BUFFER_POSITION = 2, G_BUFFER_TEX_COORD = 3, G_BUFFER_SIZE = 4
    };

    DeferredRenderer();

    uint32_t render_scene(const gl::SceneConstPtr &the_scene, const CameraPtr &the_cam,
                          const std::set<std::string> &the_tags = {}) override;


    gl::Fbo g_buffer(){ return m_geometry_fbo; }

    gl::Texture final_texture();
private:

    void init();
    void geometry_pass(const gl::vec2 &the_size, const RenderBinPtr &the_renderbin);
    void light_pass(const gl::vec2 &the_size, const RenderBinPtr &the_renderbin);

    gl::ShaderPtr m_shader_g_buffer, m_shader_g_buffer_skin;
    gl::Fbo m_geometry_fbo, m_lighting_fbo;

    gl::MaterialPtr m_mat_lighting;
    gl::MeshPtr m_mesh_sphere, m_mesh_cone;
};

}}
