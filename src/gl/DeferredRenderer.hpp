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

private:

    void create_g_buffer(const gl::vec2 &the_size, const RenderBinPtr &the_renderbin);

    gl::ShaderPtr m_shader_geometry_pass;
    gl::Fbo m_fbo;
};

}}
