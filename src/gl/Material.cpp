// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Material.hpp"

using namespace std;
using namespace glm;

namespace kinski {
namespace gl {

class InsertUniformVisitor : public boost::static_visitor<>
{
private:
    gl::ShaderPtr m_shader;
    const std::string &m_uniform;

public:

    InsertUniformVisitor(gl::ShaderPtr theShader, const std::string &theUniform)
            : m_shader(theShader), m_uniform(theUniform) {};

    template<typename T>
    inline void operator()(T &value) const
    {
        m_shader->uniform(m_uniform, value);
    }
};

Material::Material(const ShaderPtr &theShader) :
        m_shader(theShader),
        m_dirty_uniform_buffer(true),
        m_polygon_mode(GL_FRONT),
        m_wireframe(false),
        m_depth_test(true),
        m_depth_write(true),
        m_stencil_test(false),
        m_scissor_rect(crocore::Area_<uint32_t>()),
        m_blending(false),
        m_blend_src(GL_SRC_ALPHA), m_blend_dst(GL_ONE_MINUS_SRC_ALPHA),
        m_blend_equation(GL_FUNC_ADD),
        m_cull_value(CULL_BACK),
        m_shadow_properties(SHADOW_CAST | SHADOW_RECEIVE),
        m_diffuse(Color(1)),
        m_emission(gl::COLOR_BLACK),
        m_metalness(0.f),
        m_roughness(.8f),
        m_occlusion(1.f),
        m_line_width(1.f),
        m_queued_shader(ShaderType::NONE),
        m_point_size(1.f)
{
    set_point_attenuation(1.f, 0.f, 0.f);
}

MaterialPtr Material::create(const gl::ShaderType &the_type)
{
    auto ret = MaterialPtr(new Material(nullptr));
    ret->enqueue_shader(the_type);
    return ret;
}

MaterialPtr Material::create(const ShaderPtr &theShader)
{
    return MaterialPtr(new Material(theShader));
}

void Material::set_culling(uint32_t the_value)
{
    m_cull_value = the_value & (CULL_FRONT | CULL_BACK);
}

void Material::set_shadow_properties(uint32_t the_value)
{
    m_shadow_properties = the_value & (SHADOW_CAST | SHADOW_RECEIVE);
    m_dirty_uniform_buffer = true;
}

void Material::set_diffuse(const Color &theColor)
{
    m_diffuse = glm::clamp(theColor, glm::vec4(0), glm::vec4(1));
    m_dirty_uniform_buffer = true;
}

void Material::set_emission(const Color &theColor)
{
    m_emission = glm::clamp(theColor, glm::vec4(0), glm::vec4(1));
    m_dirty_uniform_buffer = true;
}

void Material::set_metalness(float m)
{
    m_metalness = crocore::clamp(m, 0.f, 1.f);
    m_dirty_uniform_buffer = true;
}

void Material::set_roughness(float r)
{
    m_roughness = crocore::clamp(r, 0.f, 1.f);
    m_dirty_uniform_buffer = true;
}

void Material::set_occlusion(float ao)
{
    m_occlusion = crocore::clamp(ao, 0.f, 1.f);
    m_dirty_uniform_buffer = true;
}

void Material::set_point_size(float sz)
{
    m_point_size = sz;
    m_dirty_uniform_buffer = true;
}

void Material::set_point_attenuation(float constant, float linear, float quadratic)
{
    m_point_attenuation = PointAttenuation(constant, linear, quadratic);
    m_dirty_uniform_buffer = true;
}

void Material::set_shader(const ShaderPtr &theShader)
{
    m_shader = theShader;
    m_queued_shader = ShaderType::NONE;
}

void Material::add_texture(const Texture &the_texture, Texture::Usage the_usage)
{
    m_textures[static_cast<uint32_t>(the_usage)] = the_texture;
    m_dirty_uniform_buffer = true;
}

void Material::clear_texture(Texture::Usage the_usage)
{
    m_textures.erase(static_cast<uint32_t>(the_usage));
    m_dirty_uniform_buffer = true;
}

void Material::add_texture(const Texture &the_texture, uint32_t the_key)
{
    m_textures[the_key] = the_texture;
    m_dirty_uniform_buffer = true;
}

void Material::clear_texture(uint32_t the_key)
{
    m_textures.erase(the_key);
    m_dirty_uniform_buffer = true;
}

bool Material::has_texture(Texture::Usage the_usage)
{
    return has_texture(static_cast<uint32_t>(the_usage));
}

bool Material::has_texture(uint32_t the_key)
{
    // search queued textures
    for(const auto &p : m_queued_textures){ if(p.second.key == the_key){ return true; }}
    return m_textures.find(the_key) != std::end(m_textures);
}

gl::Texture *Material::get_texture_ptr(Texture::Usage the_usage)
{
    auto it = m_textures.find(static_cast<uint32_t>(the_usage));
    if(it != std::end(m_textures))
    {
        return &it->second;
    }
    return nullptr;
}

gl::Texture Material::get_texture(Texture::Usage the_usage) const
{
    auto it = m_textures.find(static_cast<uint32_t>(the_usage));
    if(it != std::end(m_textures))
    {
        return it->second;
    }
    return gl::Texture();
}

glm::mat4 Material::texture_matrix() const
{
    for(const auto &pair : m_textures)
    {
        const auto &t = pair.second;
        if(!t){ continue; }
        return t.texture_matrix();
    }
    return glm::mat4(1);
}

const ShaderPtr &Material::shader()
{
    if(!m_shader && m_queued_shader != ShaderType::NONE)
    {
        set_shader(gl::create_shader(m_queued_shader));
    }
    return m_shader;
}

ShaderConstPtr Material::shader() const
{
    return m_shader;
}

void Material::enqueue_texture(const std::string &the_texture_path, uint32_t the_key)
{
    m_queued_textures[the_texture_path] = {the_key, nullptr, AssetLoadStatus::NOT_LOADED};
}

void Material::enqueue_texture(const std::string &the_texture_path, crocore::ImagePtr the_image, uint32_t the_key)
{
    if(the_image){ m_queued_textures[the_texture_path] = {the_key, the_image, AssetLoadStatus::IMAGE_LOADED}; }
    else{ enqueue_texture(the_texture_path, the_key); }
}

void Material::enqueue_shader(gl::ShaderType the_type)
{
    m_queued_shader = the_type;
}

void Material::update_uniforms(const ShaderPtr &the_shader)
{
    auto shader_obj = the_shader ? the_shader : m_shader;
    if(!shader_obj)
    {
        LOG_WARNING << "update_uniforms: no shader assigned";
        return;
    }

#if !defined(KINSKI_GLES_2)
    if(!m_uniform_buffer){ m_uniform_buffer = gl::Buffer(GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW); }


    struct material_struct_std140
    {
        vec4 diffuse;
        vec4 emission;
        vec4 point_vals;
        float metalness;
        float roughness;
        float occlusion;
        int shadow_properties;
        int texture_properties;
        int pad[3];
    };

    if(m_dirty_uniform_buffer)
    {
        material_struct_std140 m;
        m.diffuse = m_diffuse;
        m.emission = m_emission;
        m.point_vals[0] = m_point_size;
        m.point_vals[1] = m_point_attenuation.constant;
        m.point_vals[2] = m_point_attenuation.linear;
        m.point_vals[3] = m_point_attenuation.quadratic;
        m.metalness = m_metalness;
        m.roughness = m_roughness;
        m.occlusion = m_occlusion;
        m.shadow_properties = m_shadow_properties;
        m.texture_properties = 0;
        for(const auto &pair : m_textures){ m.texture_properties |= pair.first; }
        m_uniform_buffer.set_data(&m, sizeof(m));
    }
    const char *materialblock_str = "MaterialBlock";
    glBindBufferBase(GL_UNIFORM_BUFFER, gl::Context::MATERIAL_BLOCK, m_uniform_buffer.id());
    KINSKI_CHECK_GL_ERRORS();

    shader_obj->uniform_block_binding(materialblock_str, gl::Context::MATERIAL_BLOCK);
#else
    if(m_dirty_uniform_buffer)
    {
        m_uniforms["u_material.diffuse"] = m_diffuse;
        m_uniforms["u_material.emmission"] = m_emission;
        m_uniforms["u_metalness"] = m_metalness;
        m_uniforms["u_roughness"] = m_roughness;
        m_uniforms["u_material.point_vals"] = vec4(m_point_size,
                                                   m_point_attenuation.constant,
                                                   m_point_attenuation.linear,
                                                   m_point_attenuation.quadratic);
    }
#endif

    if(m_dirty_uniform_buffer)
    {
        // set all other uniform values
        for(auto&[name, uniform] : uniforms())
        {
//            std::visit([this, &name](auto &&value)
//                       {
//                            m_shader->uniform(name, value);
//                       }, uniform);
            boost::apply_visitor(InsertUniformVisitor(shader_obj, name), uniform);
            KINSKI_CHECK_GL_ERRORS();
        }
    }
    m_dirty_uniform_buffer = false;
}

}
}// namespace
