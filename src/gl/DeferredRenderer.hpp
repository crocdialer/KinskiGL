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
        G_BUFFER_EMISSION = 3,
        G_BUFFER_AO_ROUGH_METAL = 4,
        G_BUFFER_SIZE = 5
    };

    static DeferredRendererPtr create();
    uint32_t render_scene(const gl::SceneConstPtr &the_scene, const CameraPtr &the_cam,
                          const std::set<std::string> &the_tags = {}) override;


    void set_g_buffer_resolution(const gl::vec2 &the_res){ m_g_buffer_resolution = the_res; };

    bool use_fxaa() const{ return m_use_fxaa; };
    void set_use_fxaa(bool b);

    void set_enviroment_light_strength(float v){ m_enviroment_light_strength = v; };
    float enviroment_light_strength() const{ return m_enviroment_light_strength; };

    gl::FboPtr g_buffer(){ return m_fbo_geometry; }

    gl::Texture final_texture();
private:

    DeferredRenderer();
    void init();
    void geometry_pass(const gl::ivec2 &the_size, const RenderBinPtr &the_renderbin);
    void light_pass(const gl::ivec2 &the_size, const RenderBinPtr &the_renderbin);
    void stencil_pass(const RenderBinPtr &the_renderbin);
    void render_light_volumes(const RenderBinPtr &the_renderbin, bool stencil_pass);
    gl::Texture create_shadow_map(const RenderBinPtr &the_renderbin, const gl::LightPtr &l);
    gl::Texture create_env_diff(const gl::Texture &the_env_tex);
    gl::Texture create_env_spec(const gl::Texture &the_env_tex);
    gl::Texture create_brdf_lut();

    enum ShaderProperty{PROP_DEFAULT = 0, PROP_ALBEDO = 1 << 0, PROP_SKIN = 1 << 1, PROP_NORMAL = 1 << 2,
        PROP_SPEC = 1 << 3, PROP_AO_METAL_ROUGH = 1 << 4, PROP_EMMISION = 1 << 5};

    gl::vec2 m_g_buffer_resolution;
    std::map<uint32_t , gl::ShaderPtr> m_shader_map;
    
    gl::ShaderPtr m_shader_shadow, m_shader_shadow_skin, m_shader_shadow_omni, m_shader_shadow_omni_skin;
    gl::FboPtr m_fbo_geometry, m_fbo_lighting, m_fbo_shadow, m_fbo_shadow_cube;

    gl::MaterialPtr m_mat_lighting, m_mat_lighting_shadow, m_mat_lighting_shadow_omni,
    m_mat_lighting_emissive, m_mat_lighting_enviroment, m_mat_stencil, m_mat_resolve;
    gl::MeshPtr m_mesh_sphere, m_mesh_cone, m_frustum_mesh;
    
    bool m_use_fxaa = true;

    float m_enviroment_light_strength = 1.0;
    gl::MeshPtr m_skybox;
    gl::Texture m_env_conv_diff, m_env_conv_spec, m_brdf_lut;
};

}}
