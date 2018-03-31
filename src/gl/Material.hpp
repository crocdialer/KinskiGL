// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include <boost/variant.hpp>
#include "gl/gl.hpp"
#include "Shader.hpp"
#include "Buffer.hpp"
#include "Texture.hpp"

namespace kinski { namespace gl {
    
    DEFINE_CLASS_PTR(Material);
    
    class KINSKI_API Material
    {
    public:
        
        using UniformValue = boost::variant<int32_t, uint32_t, float, double, vec2, vec3, vec4,
        mat3, mat4,
        std::vector<int32_t>, std::vector<uint32_t>, std::vector<GLfloat>,
        std::vector<vec2>, std::vector<vec3>, std::vector<vec4>,
        std::vector<mat3>, std::vector<mat4>>;
        
        using UniformMap = std::unordered_map<std::string, UniformValue>;

        enum CullType{CULL_NONE = 0, CULL_FRONT = 1, CULL_BACK = 2};

        enum ShadowProperties{SHADOW_NONE = 0, SHADOW_CAST = 1, SHADOW_RECEIVE = 2};
        
        enum class TextureType{COLOR = 0, NORMAL = 1, EMISSION = 2, ROUGHNESS = 3,
            METALNESS = 4, DISPLACEMENT = 5, SHADOW = 6, DEPTH = 7, ENVIROMENT = 8, NOISE = 9,
            MASK = 10, CUSTOM_01 = 11, CUSTOM_02 = 12, CUSTOM_03 = 13, CUSTOM_04 = 14};
        
        enum class AssetLoadStatus{ NOT_LOADED = 0, LOADED = 1, NOT_FOUND = 2 };
        
        using texture_map_t = std::map<uint32_t, gl::Texture>;
        
        using texture_load_map_t = std::map<std::string, std::pair<uint32_t, AssetLoadStatus>>;
        
        static MaterialPtr create(const gl::ShaderType &the_type = gl::ShaderType::UNLIT);
        static MaterialPtr create(const ShaderPtr &theShader);
        
        bool dirty() const { return m_dirty_uniform_buffer; };
        
        void add_texture(const Texture &the_texture, TextureType the_type = TextureType::COLOR);
        void add_texture(const Texture &the_texture, uint32_t the_key);
        
        void clear_texture(TextureType the_type);
        void clear_texture(uint32_t the_key);
        
        inline void uniform(const std::string &theName, const UniformValue &theVal)
        { m_uniforms[theName] = theVal; m_dirty_uniform_buffer = true; };
        
        void update_uniforms(const ShaderPtr &the_shader = ShaderPtr());
        
        const ShaderPtr& shader();
        ShaderConstPtr shader() const;
        void set_shader(const ShaderPtr &theShader);
        
        void set_textures(const texture_map_t& the_textures)
        { m_textures = the_textures; m_dirty_uniform_buffer = true; };
        
        const texture_map_t& textures() const { return m_textures; };
        
        texture_map_t& textures(){ return m_textures; };
        
        void clear_textures(){ m_textures.clear(); }
        
        texture_load_map_t& queued_textures(){ return m_queued_textures; }
        const texture_load_map_t& queued_textures() const { return m_queued_textures; }
        
        void enqueue_texture(const std::string &the_texture_path, uint32_t the_key);
        
        std::vector<gl::ShaderType>& queued_shader(){ return m_queued_shader; }
        const std::vector<gl::ShaderType>& queued_shader() const { return m_queued_shader; }
        
        UniformMap& uniforms() {return m_uniforms;};
        const UniformMap& uniforms() const {return m_uniforms;};
        
        bool two_sided() const { return !m_cull_value; };
        bool wireframe() const { return m_wireframe; };
        bool blending() const { return m_blending; };
        GLenum blend_src() const { return m_blend_src; };
        GLenum blend_dst() const { return m_blend_dst; };
        std::pair<GLenum, GLenum> blend_factors() const { return std::make_pair(m_blend_src, m_blend_dst); };
        GLenum blend_equation() const { return m_blend_equation; };

        void set_two_sided(bool b = true) { m_cull_value = b ? CULL_NONE : CULL_BACK;};
        void set_wireframe(bool b = true) { m_wireframe = b;};
        void set_blending(bool b = true) { m_blending = b;};
        void set_blend_factors(GLenum src, GLenum dst){ m_blend_src = src; m_blend_dst = dst; };
        void set_blend_equation(GLenum equation){ m_blend_equation = equation;};
        
        void set_depth_test(bool b = true) { m_depth_test = b;};
        void set_depth_write(bool b = true) { m_depth_write = b;};

        void set_stencil_test(bool b = true) { m_stencil_test = b;};

        //! bitmask with values from CULL_FRONT, CULL_BACK
        uint32_t culling() const { return m_cull_value; }
        void set_culling(uint32_t the_value);

        uint32_t shadow_properties() const { return m_shadow_properties; }
        void set_shadow_properties(uint32_t the_value);

        bool opaque() const { return !m_blending || m_diffuse.a == 1.f ;};
        bool depth_test() const { return m_depth_test; };
        bool depth_write() const { return m_depth_write; };
        bool stencil_test() const { return m_stencil_test; };
        float point_size() const { return m_point_size; };
        
        void set_line_width(float the_line_width) { m_line_width = the_line_width; };
        float line_width() const { return m_line_width; };
        
        const Color& diffuse() const { return m_diffuse; };
        const Color& ambient() const { return m_ambient; };
        const Color& emission() const { return m_emission; };
        const float metalness() const { return m_metalness; };
        const float roughness() const { return m_roughness; };
        
        void set_diffuse(const Color &theColor);
        void set_ambient(const Color &theColor);
        void set_emission(const Color &theColor);
        void set_metalness(float m);
        void set_roughness(float r);
        void set_point_size(float sz);
        void set_point_attenuation(float constant, float linear, float quadratic);
        void set_line_thickness(float t);
        
    private:
        
        Material(const ShaderPtr &theShader);
        
        ShaderPtr m_shader;
        
        UniformMap m_uniforms;
        gl::Buffer m_uniform_buffer;
        
        bool m_dirty_uniform_buffer;
        
        GLenum m_polygon_mode;
        bool m_wireframe;
        bool m_depth_test;
        bool m_depth_write;
        bool m_stencil_test;
        bool m_blending;
        GLenum m_blend_src, m_blend_dst, m_blend_equation;

        uint32_t m_cull_value, m_shadow_properties;

        // those are available in shader
        Color m_diffuse;
        Color m_ambient;
        Color m_emission;
        float m_metalness;
        float m_roughness;
        float m_line_width;
        
        texture_load_map_t m_queued_textures;
        texture_map_t m_textures;
        
        std::vector<gl::ShaderType> m_queued_shader;
        
        // point attributes
        float m_point_size;
        struct PointAttenuation
        {
            float constant, linear, quadratic;
            PointAttenuation():constant(1.f), linear(0.f), quadratic(0.f){}
            PointAttenuation(float c, float l, float q):constant(c), linear(l), quadratic(q)
            {};
        } m_point_attenuation;
    };
}} // namespace
