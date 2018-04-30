//
// Created by Fabian on 02.05.17.
//

#include "core/ThreadPool.hpp"
#include "Mesh.hpp"
#include "Camera.hpp"
#include "Light.hpp"
#include "Scene.hpp"
#include "ShaderLibrary.h"
#include "DeferredRenderer.hpp"

namespace kinski{ namespace gl{

///////////////////////////////////////////////////////////////////////////////

DeferredRendererPtr DeferredRenderer::create()
{
    return DeferredRendererPtr(new DeferredRenderer());
}

///////////////////////////////////////////////////////////////////////////////

DeferredRenderer::DeferredRenderer()
{

}

///////////////////////////////////////////////////////////////////////////////

void DeferredRenderer::init()
{
#if !defined(KINSKI_GLES)
    std::string glsl_version = "#version 410 core";

    // default lighting
    std::string frag_src = glsl_version + "\n" + brdf_glsl + "\n" + deferred_lighting_frag;
    auto shader = gl::Shader::create(unlit_vert, frag_src);
    m_mat_lighting = gl::Material::create(shader);
    m_mat_lighting->set_depth_write(false);
    m_mat_lighting->set_stencil_test(true);
    m_mat_lighting->set_depth_test(false);
    m_mat_lighting->set_culling(Material::CULL_FRONT);
    m_mat_lighting->set_blending(true);
    m_mat_lighting->set_blend_equation(GL_FUNC_ADD);
    m_mat_lighting->set_blend_factors(GL_ONE, GL_ONE);
    
    m_mat_resolve = gl::Material::create(gl::ShaderType::RESOLVE);
    m_mat_resolve->set_blending();

    // lighting with shadowmapping
    m_mat_lighting_shadow = gl::Material::create();
    *m_mat_lighting_shadow = *m_mat_lighting;
    frag_src = glsl_version + "\n" + brdf_glsl + "\n" + deferred_lighting_shadow_frag;
    m_mat_lighting_shadow->set_shader(gl::Shader::create(unlit_vert, frag_src));
    
    // lighting with cube-shadowmapping
    m_mat_lighting_shadow_omni = gl::Material::create();
    *m_mat_lighting_shadow_omni = *m_mat_lighting;
    frag_src = glsl_version + "\n" + brdf_glsl + "\n" + deferred_lighting_shadow_omni_frag;
    m_mat_lighting_shadow_omni->set_shader(gl::Shader::create(unlit_vert, frag_src));
    
    // lighting from enviroment
    m_mat_lighting_enviroment = gl::Material::create();
    *m_mat_lighting_enviroment = *m_mat_lighting;
    m_mat_lighting_enviroment->set_shader(gl::Shader::create(unlit_vert, deferred_lighting_enviroment_frag));
    
    // lighting from emissive lighting
    m_mat_lighting_emissive = gl::Material::create();
    m_mat_lighting_emissive->set_depth_test(false);
    m_mat_lighting_emissive->set_depth_write(false);
    m_mat_lighting_emissive->set_blending(true);
    m_mat_lighting_emissive->set_blend_equation(GL_FUNC_ADD);
    m_mat_lighting_emissive->set_blend_factors(GL_ONE, GL_ONE);
    
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

    // create our shaders
    m_shader_map[PROP_DEFAULT] = m_shader_map[PROP_ALBEDO] = gl::Shader::create(phong_vert, create_g_buffer_frag);
    m_shader_map[PROP_SKIN] = m_shader_map[PROP_SKIN | PROP_ALBEDO] = gl::Shader::create(phong_skin_vert, create_g_buffer_frag);
    m_shader_map[PROP_ALBEDO | PROP_NORMAL] = gl::Shader::create(phong_tangent_vert, create_g_buffer_normalmap_frag);
    m_shader_map[PROP_ALBEDO | PROP_SKIN | PROP_NORMAL] = gl::Shader::create(phong_tangent_skin_vert,
                                                                             create_g_buffer_normalmap_frag);
    m_shader_map[PROP_ALBEDO | PROP_NORMAL | PROP_SPEC] = gl::Shader::create(phong_tangent_vert,
                                                                             create_g_buffer_normal_spec_frag);
    m_shader_map[PROP_AO_METAL_ROUGH] = gl::Shader::create(phong_vert, create_g_buffer_rough_frag);
    m_shader_map[PROP_ALBEDO | PROP_AO_METAL_ROUGH] = gl::Shader::create(phong_vert, create_g_buffer_color_rough_frag);
    m_shader_map[PROP_ALBEDO | PROP_NORMAL | PROP_AO_METAL_ROUGH] = gl::Shader::create(phong_tangent_vert,
                                                                                       create_g_buffer_normal_rough_frag);
    m_shader_map[PROP_ALBEDO | PROP_NORMAL | PROP_AO_METAL_ROUGH | PROP_EMMISION] =
            gl::Shader::create(phong_tangent_vert, create_g_buffer_normal_rough_emmision_frag);
    m_shader_map[PROP_ALBEDO | PROP_SKIN | PROP_NORMAL | PROP_SPEC] = gl::Shader::create(phong_tangent_skin_vert,
                                                                                         create_g_buffer_normal_spec_frag);

    m_shader_map[PROP_ALBEDO | PROP_SKIN | PROP_NORMAL | PROP_AO_METAL_ROUGH] = gl::Shader::create(phong_tangent_skin_vert,
                                                                                                   create_g_buffer_normal_rough_frag);
    m_shader_shadow = gl::Shader::create(empty_vert, empty_frag);
    m_shader_shadow_skin = gl::Shader::create(empty_skin_vert, empty_frag);
    
    m_shader_shadow_omni = gl::Shader::create(empty_vert, linear_depth_frag, cube_layers_geom);
    m_shader_shadow_omni_skin = gl::Shader::create(empty_skin_vert, linear_depth_frag, cube_layers_geom);
    
    const uint32_t sz = 1024, cube_sz = 512;
    
    // create shadow fbo (spot + directional lights)
    gl::Fbo::Format fbo_fmt;
    fbo_fmt.set_num_color_buffers(0);
    m_fbo_shadow = gl::Fbo(sz, sz, fbo_fmt);
    
    // create shadow fbo (point lights)
    m_fbo_shadow_cube = create_cube_framebuffer(cube_sz, false);
    KINSKI_CHECK_GL_ERRORS();
#endif
}

///////////////////////////////////////////////////////////////////////////////

uint32_t DeferredRenderer::render_scene(const gl::SceneConstPtr &the_scene, const CameraPtr &the_cam,
                                        const std::set<std::string> &the_tags)
{
#if !defined(KINSKI_GLES)

    if(!m_mat_lighting){ init(); }

    gl::reset_state();

    // determine internal resolution for rendering
    gl::vec2 resolution = gl::window_dimension();
    if(m_g_buffer_resolution.x > 0 && m_g_buffer_resolution.y > 0){ resolution = m_g_buffer_resolution; }

    // culling
    auto render_bin = cull(the_scene, the_cam, the_tags);

    {
        gl::SaveFramebufferBinding sfb;

        // create G-buffer, if necessary, and fill it
        geometry_pass(resolution, render_bin);

        // lighting pass
        light_pass(resolution, render_bin);
    }

    // skybox drawing
    if(the_scene->skybox())
    {
        gl::ScopedMatrixPush mv(gl::MODEL_VIEW_MATRIX), proj(gl::PROJECTION_MATRIX);
        gl::set_projection(the_cam);
        mat4 m = the_cam->view_matrix();
        m[3] = vec4(0, 0, 0, 1);
        gl::load_matrix(gl::MODEL_VIEW_MATRIX, glm::scale(m, gl::vec3(the_cam->far() * .99f)));
        gl::draw_mesh(the_scene->skybox());
    }
    // draw light texture
    m_mat_resolve->add_texture(m_fbo_lighting.texture());
    m_mat_resolve->add_texture(m_fbo_geometry.depth_texture(), Texture::Usage::DEPTH);
    m_mat_resolve->uniform("u_use_fxaa", m_use_fxaa ? 1 : 0);
    m_mat_resolve->uniform("u_luma_thresh", .4f);
    gl::draw_quad(gl::window_dimension(), m_mat_resolve);
    
    // draw emission texture
    gl::draw_quad(gl::window_dimension(), m_mat_lighting_emissive);
    
    // return number of rendered objects
    return render_bin->items.size();
#endif
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

void DeferredRenderer::geometry_pass(const gl::ivec2 &the_size, const RenderBinPtr &the_renderbin)
{
#if !defined(KINSKI_GLES)

    if(!m_fbo_geometry || m_fbo_geometry.size() != the_size)
    {
        gl::Fbo::Format fmt;
        fmt.set_color_internal_format(GL_RGB16F);
        fmt.enable_stencil_buffer(true);
        fmt.set_num_color_buffers(G_BUFFER_SIZE);
        m_fbo_geometry = gl::Fbo(the_size, fmt);

        for(uint32_t i = 0; i < G_BUFFER_SIZE; ++i)
        {
            m_fbo_geometry.texture(i).set_mag_filter(GL_NEAREST);
            m_fbo_geometry.texture(i).set_min_filter(GL_NEAREST);
        }
        KINSKI_CHECK_GL_ERRORS();
        
        m_mat_lighting_emissive->add_texture(m_fbo_geometry.texture(G_BUFFER_EMISSION),
                                             Texture::Usage::COLOR);
    }

    std::list<RenderBin::item> opaque_items, blended_items;
    sort_render_bin(the_renderbin, opaque_items, blended_items);
    
    // tmp hack to draw all geometry
    opaque_items.insert(opaque_items.end(), blended_items.begin(), blended_items.end());
    
    // bind G-Buffer
    gl::SaveViewPort sv;
    gl::set_window_dimension(m_fbo_geometry.size());
    m_fbo_geometry.bind();
    gl::clear();
    KINSKI_CHECK_GL_ERRORS();
    
    auto select_shader = [this](const gl::MeshPtr &m) -> gl::ShaderPtr
    {
        bool has_albedo = m->material()->has_texture(Texture::Usage::COLOR);
        bool has_normal_map = m->material()->has_texture(Texture::Usage::NORMAL);
        bool has_spec_map = m->material()->has_texture(Texture::Usage::SPECULAR);
        bool has_ao_rough_metal_map = m->material()->has_texture(Texture::Usage::AO_ROUGHNESS_METAL);
        bool has_emmision_map = m->material()->has_texture(Texture::Usage::EMISSION);

        uint32_t key = PROP_DEFAULT;
        if(has_albedo){ key |= PROP_ALBEDO; }
        if(m->root_bone()){ key |= PROP_SKIN; }
        if(has_normal_map){ key |= PROP_NORMAL; }
        if(has_emmision_map){ key |= PROP_EMMISION; }
        if(has_ao_rough_metal_map){ key |= PROP_AO_METAL_ROUGH; }
        else if(has_spec_map){ key |= PROP_SPEC; }
        return m_shader_map[key];
    };
    
    for (const RenderBin::item &item : opaque_items)
    {
        gl::ScopedMatrixPush mv(gl::MODEL_VIEW_MATRIX), proj(gl::PROJECTION_MATRIX);
        gl::set_projection(the_renderbin->camera);
        gl::load_matrix(gl::MODEL_VIEW_MATRIX, item.transform);
        gl::ShaderPtr shader = select_shader(item.mesh);
        gl::draw_mesh(item.mesh, shader);
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////

void DeferredRenderer::light_pass(const gl::ivec2 &the_size, const RenderBinPtr &the_renderbin)
{
#if !defined(KINSKI_GLES)

    if(!m_fbo_lighting || m_fbo_lighting.size() != m_fbo_geometry.size())
    {
        gl::Fbo::Format fmt;
        fmt.enable_stencil_buffer(true);
        m_fbo_lighting = gl::Fbo(m_fbo_geometry.size(), fmt);
        m_fbo_lighting.set_depth_texture(m_fbo_geometry.depth_texture());
        KINSKI_CHECK_GL_ERRORS();

        m_mat_lighting->clear_textures();
        m_mat_lighting_shadow->clear_textures();
        m_mat_lighting_shadow_omni->clear_textures();
        
        uint32_t i = 0;
        for(; i < G_BUFFER_SIZE; ++i)
        {
            m_mat_lighting->add_texture(m_fbo_geometry.texture(i), i);
            m_mat_lighting_shadow->add_texture(m_fbo_geometry.texture(i), i);
            m_mat_lighting_shadow_omni->add_texture(m_fbo_geometry.texture(i), i);
            m_mat_lighting_enviroment->add_texture(m_fbo_geometry.texture(i), i);
        }
        m_mat_lighting->shader()->uniform_block_binding("LightBlock", LIGHT_BLOCK);
        m_mat_lighting_shadow->shader()->uniform_block_binding("LightBlock", LIGHT_BLOCK);
        m_mat_lighting_shadow_omni->shader()->uniform_block_binding("LightBlock", LIGHT_BLOCK);
    }
    gl::SaveViewPort sv;
    gl::set_window_dimension(m_fbo_lighting.size());
    m_fbo_lighting.bind();

    // update frustum for directional and eviroment lights
    m_frustum_mesh = gl::create_frustum_mesh(the_renderbin->camera, true);

    gl::apply_material(m_mat_lighting);
    update_uniform_buffers(the_renderbin->lights);


    auto c = gl::COLOR_BLACK;
    c.a = 0.f;
    gl::clear(c);

    // stencil pass
    glStencilFunc(GL_ALWAYS, 0, 0);
    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
    m_fbo_lighting.enable_draw_buffers(false);
    render_light_volumes(the_renderbin, true);

    // light pass
    glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
    m_fbo_lighting.enable_draw_buffers(true);
    render_light_volumes(the_renderbin, false);
#endif
}

///////////////////////////////////////////////////////////////////////////////

gl::Texture DeferredRenderer::create_shadow_map(const RenderBinPtr &the_renderbin,
                                                const gl::LightPtr &l)
{
#if !defined(KINSKI_GLES)
    auto shadow_cam = gl::create_shadow_camera(l, l->radius());
    auto bin = cull(the_renderbin->scene, shadow_cam);
    std::list<RenderBin::item> opaque_items, blended_items;
    sort_render_bin(bin, opaque_items, blended_items);
    
    if(l->type() == gl::Light::SPOT || l->type() == gl::Light::DIRECTIONAL)
    {
        // offscreen render shadow map here
        gl::render_to_texture(m_fbo_shadow, [&]()
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
        mat4 shadow_matrix = shadow_cam->projection_matrix() * shadow_cam->view_matrix() *
        the_renderbin->camera->global_transform();
        m_mat_lighting_shadow->uniform("u_shadow_matrix", shadow_matrix);
        return m_fbo_shadow.depth_texture();
    }
    else if(l->type() == gl::Light::POINT)
    {
        KINSKI_CHECK_GL_ERRORS();
        
        auto cube_cam = std::dynamic_pointer_cast<CubeCamera>(shadow_cam);
        
        std::vector<glm::mat4> cam_matrices = cube_cam->view_matrices();
        
        m_shader_shadow_omni->bind();
        m_shader_shadow_omni->uniform("u_view_matrix", cam_matrices);
        m_shader_shadow_omni->uniform("u_projection_matrix", cube_cam->projection_matrix());
        m_shader_shadow_omni->uniform("u_clip_planes", vec2(cube_cam->near(), cube_cam->far()));
        m_shader_shadow_omni_skin->bind();
        m_shader_shadow_omni_skin->uniform("u_view_matrix", cam_matrices);
        m_shader_shadow_omni_skin->uniform("u_projection_matrix", cube_cam->projection_matrix());
        m_shader_shadow_omni_skin->uniform("u_clip_planes", vec2(cube_cam->near(), cube_cam->far()));
        KINSKI_CHECK_GL_ERRORS();
        
        // offscreen render shadow map here
        gl::render_to_texture(m_fbo_shadow_cube, [&]()
        {
            gl::reset_state();
            glClear(GL_DEPTH_BUFFER_BIT);
            gl::ScopedMatrixPush mv(gl::MODEL_VIEW_MATRIX), proj(gl::PROJECTION_MATRIX);
            gl::load_identity(gl::PROJECTION_MATRIX);
          
            for(const RenderBin::item &item : opaque_items)
            {
                // filter out non-shadow casters
                if(!(item.mesh->material()->shadow_properties() & gl::Material::SHADOW_CAST)){ continue; }
              
                gl::load_matrix(gl::MODEL_VIEW_MATRIX, item.mesh->global_transform());
                gl::ShaderPtr shader = item.mesh->geometry()->has_bones() ?
                    m_shader_shadow_omni_skin : m_shader_shadow_omni;
                gl::draw_mesh(item.mesh, shader);
            }
        });
        m_mat_lighting_shadow_omni->uniform("u_clip_planes", vec2(cube_cam->near(), cube_cam->far()));
        m_mat_lighting_shadow_omni->uniform("u_camera_transform",
                                            the_renderbin->camera->global_transform());
        m_mat_lighting_shadow_omni->uniform("u_poisson_radius", .005f);
        return m_fbo_shadow_cube.depth_texture();
    }
#endif
    return gl::Texture();
}

///////////////////////////////////////////////////////////////////////////////

void DeferredRenderer::render_light_volumes(const RenderBinPtr &the_renderbin, bool stencil_pass)
{
    gl::ScopedMatrixPush mv(gl::MODEL_VIEW_MATRIX), proj(gl::PROJECTION_MATRIX);
    gl::set_projection(the_renderbin->camera);

    gl::MaterialPtr mat = stencil_pass ? m_mat_stencil : m_mat_lighting;
    int light_index = 0;

    for(auto l : the_renderbin->lights)
    {
        if(!stencil_pass)
        {
            if(l.light->cast_shadow())
            {
                auto shadow_map = create_shadow_map(the_renderbin, l.light);
                
                if(shadow_map)
                {
                    mat = (l.light->type() == gl::Light::POINT) ?
                        m_mat_lighting_shadow_omni : m_mat_lighting_shadow;
                    mat->add_texture(shadow_map, Texture::Usage::SHADOW);
                }
            }
            else{ mat = m_mat_lighting; }
        }
        m_frustum_mesh->material() = m_mesh_sphere->material() = m_mesh_cone->material() = mat;
        float d = std::min(the_renderbin->camera->far() * 0.66f + l.transform[3].z, l.light->radius());
        d = std::max(d, 1.f);

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
            default:
                LOG_WARNING << "light type not handled";
                break;
        }
    }
    // enviroment lighting / reflection
    if(the_renderbin->scene->skybox())
    {
        auto t = the_renderbin->scene->skybox()->material()->get_texture(Texture::Usage::ENVIROMENT);

        if(t && t.target() == GL_TEXTURE_CUBE_MAP)
        {
            if(m_skybox != the_renderbin->scene->skybox())
            {
                if(!m_brdf_lut){ m_brdf_lut = create_brdf_lut(); }
                m_env_conv_diff = create_env_diff(t);
                m_env_conv_spec = create_env_spec(t);
                m_skybox = the_renderbin->scene->skybox();

                m_mat_lighting_enviroment->add_texture(m_env_conv_diff, Texture::Usage::ENVIROMENT_CONV_DIFF);
                m_mat_lighting_enviroment->add_texture(m_env_conv_spec, Texture::Usage::ENVIROMENT_CONV_SPEC);
                m_mat_lighting_enviroment->add_texture(m_brdf_lut, Texture::Usage::BRDF_LUT);

                // gamma correction for HDR backgrounds (HDR -> LDR)
                float gamma = t.datatype() == GL_FLOAT ? 2.2f : 1.f;
                the_renderbin->scene->skybox()->material()->uniform("u_gamma", gamma);

                // use this to use spec convolution as background for debugging
//                the_renderbin->scene->skybox()->material()->add_texture(m_env_conv_spec, Texture::Usage::ENVIROMENT);
            }

            if(!stencil_pass)
            {
                m_mat_lighting_enviroment->uniform("u_camera_transform", the_renderbin->camera->global_transform());
                m_mat_lighting_enviroment->uniform("u_env_light_strength", m_enviroment_light_strength);
                m_frustum_mesh->material() = m_mat_lighting_enviroment;
            }else{ m_frustum_mesh->material() = m_mat_stencil; }
            gl::load_identity(gl::MODEL_VIEW_MATRIX);
            gl::draw_mesh(m_frustum_mesh);
        }
    }
}
    
///////////////////////////////////////////////////////////////////////////////
    
gl::Texture DeferredRenderer::create_env_diff(const gl::Texture &the_env_tex)
{
    auto task = Task::create("cubemap diffuse convolution");
    constexpr uint32_t conv_size = 32;
    constexpr GLenum data_type = GL_FLOAT;

    auto mat = gl::Material::create();
    mat->set_culling(gl::Material::CULL_FRONT);
    mat->add_texture(the_env_tex, Texture::Usage::ENVIROMENT);
    mat->set_depth_test(false);
    mat->set_depth_write(false);
    auto box_mesh = gl::Mesh::create(gl::Geometry::create_box(gl::vec3(.5f)), mat);

    // render enviroment cubemap here
    auto cube_cam = gl::CubeCamera::create(.1f, 10.f);
    auto cube_fbo = gl::create_cube_framebuffer(conv_size, true, data_type);
    auto cube_shader = gl::Shader::create(cube_vert, cube_conv_diffuse_frag, cube_layers_env_geom);

    auto cam_matrices = cube_cam->view_matrices();

    cube_shader->bind();
    cube_shader->uniform("u_view_matrix", cam_matrices);
    cube_shader->uniform("u_projection_matrix", cube_cam->projection_matrix());

    auto cube_tex = gl::render_to_texture(cube_fbo, [box_mesh, cube_shader]()
    {
        gl::draw_mesh(box_mesh, cube_shader);
    });
    cube_tex.set_mag_filter(GL_LINEAR);
    KINSKI_CHECK_GL_ERRORS();

    return cube_tex;
}

///////////////////////////////////////////////////////////////////////////////
    
gl::Texture DeferredRenderer::create_env_spec(const gl::Texture &the_env_tex)
{
    auto task = Task::create("cubemap specular convolution");
    constexpr uint32_t conv_size = 512;
    constexpr uint32_t num_color_components = 3;
    constexpr GLenum data_type = GL_FLOAT;

    uint32_t num_mips = std::log2(conv_size) - 1;
    m_mat_lighting_enviroment->shader()->uniform("u_num_mip_levels", num_mips);

    auto mat = gl::Material::create();
    mat->set_culling(gl::Material::CULL_FRONT);
    mat->add_texture(the_env_tex, Texture::Usage::ENVIROMENT);
    mat->set_depth_test(false);
    mat->set_depth_write(false);
    auto box_mesh = gl::Mesh::create(gl::Geometry::create_box(gl::vec3(.5f)), mat);

    GLenum format = 0, internal_format = 0;
    get_texture_format(num_color_components, false, data_type, &format, &internal_format);

    gl::Texture::Format fmt;
    fmt.set_target(GL_TEXTURE_CUBE_MAP);
    fmt.set_internal_format(internal_format);
    fmt.set_data_type(data_type);
    auto ret = gl::Texture(nullptr, format, conv_size, conv_size, fmt);
    ret.set_mipmapping(true);

    // render enviroment cubemap here
    auto cube_cam = gl::CubeCamera::create(.1f, 10.f);
    auto cube_shader = gl::Shader::create(cube_vert, cube_conv_spec_frag, cube_layers_env_geom);
    auto cam_matrices = cube_cam->view_matrices();

    cube_shader->bind();
    cube_shader->uniform("u_view_matrix", cam_matrices);
    cube_shader->uniform("u_projection_matrix", cube_cam->projection_matrix());

    for(uint32_t lvl = 0; lvl < num_mips; ++lvl)
    {
        size_t mip_size = conv_size >> lvl;
        auto cube_fbo = gl::create_cube_framebuffer(mip_size, true, data_type);
        cube_shader->uniform("u_roughness", (float)lvl / num_mips);

        auto cube_tex = gl::render_to_texture(cube_fbo, [box_mesh, cube_shader]()
        {
            gl::draw_mesh(box_mesh, cube_shader);
        });
        KINSKI_CHECK_GL_ERRORS();

        // create PBO
        size_t num_bytes_per_comp = data_type == GL_FLOAT ? 4 : data_type == GL_UNSIGNED_SHORT ? 2 : 1;
        gl::Buffer pixel_buf = gl::Buffer(GL_PIXEL_PACK_BUFFER, GL_STATIC_COPY);
        pixel_buf.set_data(nullptr, mip_size * mip_size * 4 * num_bytes_per_comp);

        // bind PBO for reading and writing
        pixel_buf.bind(GL_PIXEL_PACK_BUFFER);
        pixel_buf.bind(GL_PIXEL_UNPACK_BUFFER);

        // copy cube_tex to current mipmap
        for(uint32_t i = 0; i < 6; i++)
        {
            // copy data to PBO
            cube_tex.bind();
            glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, cube_tex.datatype(), nullptr);
            KINSKI_CHECK_GL_ERRORS();

            // copy data from PBO to appropriate image plane and mimap lvl
            ret.bind();
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, lvl, internal_format, mip_size, mip_size, 0, format,
                         ret.datatype(), nullptr);
            KINSKI_CHECK_GL_ERRORS();
        }
    }
    ret.set_mag_filter(GL_LINEAR);
    return ret;
}
    
///////////////////////////////////////////////////////////////////////////////

gl::Texture DeferredRenderer::create_brdf_lut()
{
    auto task = Task::create("BRDF-lut baking");
    constexpr uint32_t tex_size = 512;
    gl::Fbo::Format fmt;
    fmt.set_color_internal_format(GL_RG32F);
    auto fbo = gl::Fbo(tex_size, tex_size, fmt);
    auto gen_brdf_shader = gl::Shader::create(unlit_vert, gen_brdf_lut_frag);
    auto mat = gl::Material::create(gen_brdf_shader);
    mat->set_depth_test(false);
    mat->set_depth_write(false);

    auto ret = gl::render_to_texture(fbo, [mat]()
    {
        gl::draw_quad(gl::window_dimension(), mat);
    });
    return ret;
}

///////////////////////////////////////////////////////////////////////////////

gl::Texture DeferredRenderer::final_texture()
{
    return m_fbo_lighting.texture();
}

///////////////////////////////////////////////////////////////////////////////

void DeferredRenderer::set_use_fxaa(bool b)
{
    m_use_fxaa = b;
};

///////////////////////////////////////////////////////////////////////////////

}}
