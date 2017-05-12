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

namespace kinski { namespace gl {
    
    class InsertUniformVisitor : public boost::static_visitor<>
    {
    private:
        gl::ShaderPtr m_shader;
        const std::string &m_uniform;
        
    public:
        
        InsertUniformVisitor(gl::ShaderPtr theShader, const std::string &theUniform)
        :m_shader(theShader), m_uniform(theUniform){};
        
        template <typename T>
        void operator()( T &value ) const
        {
            m_shader->uniform(m_uniform, value);
        }
    };
    
    Material::Material(const ShaderPtr &theShader):
    m_shader(theShader),
    m_dirty_uniform_buffer(true),
    m_polygon_mode(GL_FRONT),
    m_wireframe(false),
    m_depth_test(true),
    m_depth_write(true),
    m_stencil_test(false),
    m_blending(false),
    m_blend_src(GL_SRC_ALPHA), m_blend_dst(GL_ONE_MINUS_SRC_ALPHA),
    m_blend_equation(GL_FUNC_ADD),
    m_cull_value(CULL_BACK),
    m_shadow_properties(SHADOW_CAST),
    m_diffuse(Color(1)),
    m_ambient(Color(1)),
    m_specular(Color(1)),
    m_emission(Color(0)),
    m_shinyness(10.0f),
    m_line_width(1.f),
    m_point_size(1.f)
    {
        set_point_attenuation(1.f, 0.f, 0.f);
    }
    
    MaterialPtr Material::create(const gl::ShaderType &the_type)
    {
//        auto ret = MaterialPtr(new Material(nullptr));
//        ret->load_queue_shader().push_back(the_type);
//        return ret;
        return MaterialPtr(new Material(gl::create_shader(the_type)));
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
    
    void Material::set_ambient(const Color &theColor)
    {
        m_ambient = glm::clamp(theColor, glm::vec4(0), glm::vec4(1));
        m_dirty_uniform_buffer = true;
    }
    
    void Material::set_specular(const Color &theColor)
    {
        m_specular = glm::clamp(theColor, glm::vec4(0), glm::vec4(1));
        m_dirty_uniform_buffer = true;
    }
    
    void Material::set_emission(const Color &theColor)
    {
        m_emission = glm::clamp(theColor, glm::vec4(0), glm::vec4(1));
        m_dirty_uniform_buffer = true;
    }
    
    void Material::set_shinyness(float s)
    {
        m_shinyness = s;
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
    };
    
    void Material::set_shader(const ShaderPtr &theShader)
    {
        m_shader = theShader;
        m_load_queue_shader.clear();
    };
    
    void Material::queue_texture_load(const std::string &the_texture_path)
    {
        m_texture_paths[the_texture_path] = AssetLoadStatus::NOT_LOADED;
    }
    
    void Material::update_uniforms(const ShaderPtr &the_shader)
    {
        auto shader_obj = the_shader ? the_shader : m_shader;

#if !defined(KINSKI_GLES)
        if(!m_uniform_buffer){ m_uniform_buffer = gl::Buffer(GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW); }
        
        
        struct material_struct_std140
        {
            vec4 diffuse;
            vec4 ambient;
            vec4 specular;
            vec4 emission;
            vec4 point_vals;
            float shinyness;
            int shadow_properties;
            uint32_t pad[2];
        };
        
        if(m_dirty_uniform_buffer)
        {
            material_struct_std140 m;
            m.diffuse = m_diffuse;
            m.ambient = m_ambient;
            m.specular = m_specular;
            m.emission = m_emission;
            m.point_vals[0] = m_point_size;
            m.point_vals[1] = m_point_attenuation.constant;
            m.point_vals[2] = m_point_attenuation.linear;
            m.point_vals[3] = m_point_attenuation.quadratic;
            m.shinyness = m_shinyness;
            m.shadow_properties = m_shadow_properties;

            m_uniform_buffer.set_data(&m, sizeof(m));
            m_dirty_uniform_buffer = false;
        }
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_uniform_buffer.id());
        KINSKI_CHECK_GL_ERRORS();

        if(shader_obj){ shader_obj->uniform_block_binding("MaterialBlock", 0); }
#else
        if(m_dirty_uniform_buffer)
        {
            m_uniforms["u_material.diffuse"] = m_diffuse;
            m_uniforms["u_material.ambient"] = m_ambient;
            m_uniforms["u_material.specular"] = m_specular;
            m_uniforms["u_material.emmission"] = m_emission;
            m_uniforms["u_material.shinyness"] = m_shinyness;
            m_uniforms["u_material.point_vals"] = vec4(m_point_size,
                                                       m_point_attenuation.constant,
                                                       m_point_attenuation.linear,
                                                       m_point_attenuation.quadratic);
            m_dirty_uniform_buffer = false;
        }
#endif
        
//        if(m_dirty_uniform_buffer)
        {
            // set all other uniform values
            for (auto it = uniforms().begin(); it != uniforms().end(); ++it)
            {
                boost::apply_visitor(InsertUniformVisitor(shader_obj, it->first), it->second);
                KINSKI_CHECK_GL_ERRORS();
            }
        }
    }
    
}}// namespace
