//
// Created by crocdialer on 10/9/18.
//

#include "Blur.hpp"
#include "gl/Material.hpp"

namespace kinski { namespace gl {

struct BlurImpl
{
    glm::vec2 m_scale = glm::vec2(3.f);
    gl::MaterialPtr m_material = gl::Material::create(gl::ShaderType::BLUR);
};

Blur::Blur(const glm::vec2 &the_scale):
m_impl(std::make_shared<BlurImpl>())
{
    m_impl->m_scale = the_scale;
    m_impl->m_material->set_blending();
    m_impl->m_material->set_depth_test(false);
    m_impl->m_material->set_depth_write(false);
}

void Blur::render_output(const gl::Texture &the_texture)
{
    m_impl->m_material->uniform("u_poisson_radius", m_impl->m_scale);
    m_impl->m_material->add_texture(the_texture);
    gl::draw_quad(gl::window_dimension(), m_impl->m_material);
}

glm::vec2 Blur::scale() const
{
    return m_impl->m_scale;
}

void Blur::set_scale(const glm::vec2 &the_scale)
{
    m_impl->m_scale = the_scale;
}

}}//namespaces