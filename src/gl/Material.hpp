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
    
    class KINSKI_API Material
    {
    public:
        
        typedef std::shared_ptr<Material> Ptr;
        typedef std::shared_ptr<const Material> ConstPtr;
        typedef std::weak_ptr<Material> WeakPtr;
        
        typedef boost::variant<GLint, GLfloat, double, vec2, vec3, vec4,
        mat3, mat4,
        std::vector<GLint>, std::vector<GLfloat>,
        std::vector<vec2>, std::vector<vec3>, std::vector<vec4>,
        std::vector<mat3>, std::vector<mat4> > UniformValue;
        
        typedef std::unordered_map<std::string, UniformValue> UniformMap;
        
        enum class AssetLoadStatus{ NOT_LOADED, LOADED, NOT_FOUND };
        
        static Ptr create(const gl::ShaderType &the_type = gl::ShaderType::UNLIT);
        static Ptr create(const Shader &theShader);        
        
        bool dirty() const { return m_dirty_uniform_buffer; };
        
        void add_texture(const Texture &theTexture) {m_textures.push_back(theTexture);};
        
        inline void uniform(const std::string &theName, const UniformValue &theVal)
        { m_uniforms[theName] = theVal; m_dirty_uniform_buffer = true; };
        
        void update_uniforms();
        
        Shader& shader() {return m_shader;};
        const Shader& shader() const {return m_shader;};
        void set_shader(const Shader &theShader);
        
        std::vector<Texture>& textures() {return m_textures;};
        const std::vector<Texture>& textures() const {return m_textures;};
        
        std::map<std::string, AssetLoadStatus>& texture_paths(){ return m_texture_paths; }
        const std::map<std::string, AssetLoadStatus>& texture_paths() const { return m_texture_paths; }
        
        void queue_texture_load(const std::string &the_texture_path);
        
        std::vector<gl::ShaderType>& load_queue_shader(){ return m_load_queue_shader; }
        const std::vector<gl::ShaderType>& load_queue_shader() const { return m_load_queue_shader; }
        
        UniformMap& uniforms() {return m_uniforms;};
        const UniformMap& uniforms() const {return m_uniforms;};
        
        bool two_sided() const { return m_two_sided; };
        bool wireframe() const { return m_wireframe; };
        bool blending() const { return m_blending; };
        GLenum blend_src() const { return m_blend_src; };
        GLenum blend_dst() const { return m_blend_dst; };
        GLenum blend_equation() const { return m_blend_equation; };
        
        void set_two_sided(bool b = true) { m_two_sided = b;};
        void set_wireframe(bool b = true) { m_wireframe = b;};
        void set_blending(bool b = true) { m_blending = b;};
        void set_blend_factors(GLenum src, GLenum dst){ m_blend_src = src; m_blend_dst = dst; };
        void set_blend_equation(GLenum equation){ m_blend_equation = equation;};
        
        void set_depth_test(bool b = true) { m_depth_test = b;};
        void set_depth_write(bool b = true) { m_depth_write = b;};
        
        bool opaque() const { return !m_blending && m_diffuse.a == 1.f ;};
        bool depth_test() const { return m_depth_test; };
        bool depth_write() const { return m_depth_write; };
        float point_size() const { return m_point_size; };
        
        void set_line_width(float the_line_width) { m_line_width = the_line_width; };
        float line_width() const { return m_line_width; };
        
        const Color& diffuse() const { return m_diffuse; };
        const Color& ambient() const { return m_ambient; };
        const Color& specular() const { return m_specular; };
        const Color& emission() const { return m_emission; };
        const float shinyness() const { return m_shinyness; };
        
        void set_diffuse(const Color &theColor);
        void set_ambient(const Color &theColor);
        void set_specular(const Color &theColor);
        void set_emission(const Color &theColor);
        void set_shinyness(float s);
        void set_point_size(float sz);
        void set_point_attenuation(float constant, float linear, float quadratic);
        void set_line_thickness(float t);
        
    private:
        
        Material(const Shader &theShader);
        
        Shader m_shader;
        
        UniformMap m_uniforms;
        gl::Buffer m_uniform_buffer;
        
        bool m_dirty_uniform_buffer;
        
        GLenum m_polygon_mode;
        bool m_two_sided;
        bool m_wireframe;
        bool m_depth_test;
        bool m_depth_write;
        bool m_blending;
        GLenum m_blend_src, m_blend_dst, m_blend_equation;
        
        // those are available in shader
        Color m_diffuse;
        Color m_ambient;
        Color m_specular;
        Color m_emission;
        float m_shinyness;
        
        float m_line_width;
        
        std::map<std::string, AssetLoadStatus> m_texture_paths;
        std::vector<Texture> m_textures;
        
        std::vector<gl::ShaderType> m_load_queue_shader;
        
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
