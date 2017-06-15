// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  Noise.cpp
//
//  Created by Croc Dialer on 25/08/15.

#include "QuadWarp.hpp"
#include "gl/Mesh.hpp"
#include "gl/Camera.hpp"

namespace kinski{ namespace gl{
    
    const gl::ivec2 QuadWarp::s_max_num_subdivisions = gl::ivec2(5);
    
    namespace
    {
        const std::vector<gl::vec2> default_points = {gl::vec2(0, 0), gl::vec2(1, 0), gl::vec2(0, 1),
            gl::vec2(1, 1)};
    };
    
    struct Impl
    {
        uint32_t m_grid_num_w = 32, m_grid_num_h = 18;
        
        ivec2 m_num_subdivisions = ivec2(1, 1);
        
        std::vector<gl::vec2> m_control_points;
        std::set<uint32_t> m_selected_indices;
        
        gl::OrthographicCamera::Ptr
        m_camera = gl::OrthographicCamera::create(0, 1, 0, 1, 0, 1);
        
        gl::MeshPtr m_mesh, m_grid_mesh;
        gl::ShaderPtr m_shader_warp_vert, m_shader_warp_vert_rect;
        
        Area_<uint32_t> m_src_area;
        
        Impl(uint32_t the_res_w = 32, uint32_t the_res_h = 18):
        m_grid_num_w(the_res_w), m_grid_num_h(the_res_h)
        {
            m_control_points = default_points;
            
            // adjust grid density here
            auto geom = gl::Geometry::create_plane(1, 1, m_grid_num_w, m_grid_num_h);
            for(auto &v : geom->vertices()){ v += vec3(0.5f, 0.5f, 0.f); }
            geom->compute_bounding_box();
            
            try
            {
                m_shader_warp_vert = gl::create_shader(gl::ShaderType::QUAD_WARP);
#if !defined(KINSKI_GLES)
                m_shader_warp_vert_rect = gl::create_shader(gl::ShaderType::QUAD_WARP_RECT);
#else
                m_shader_warp_vert_rect = m_shader_warp_vert;
#endif
            }
            catch(Exception &e){ LOG_ERROR << e.what(); }
            
            auto mat = gl::Material::create(m_shader_warp_vert);
            mat->set_depth_test(false);
            mat->set_depth_write(false);
            mat->set_blending(true);
//            mat->set_wireframe();
            m_mesh = gl::Mesh::create(geom, mat);
            
            auto grid_geom = gl::Geometry::create_grid(1.f, 1.f, m_grid_num_w, m_grid_num_h);
            for(auto &v : grid_geom->vertices())
            {
                v = vec3(v.x, -v.z, v.y) + vec3(0.5f, 0.5f, 0.f);
            }
            for(auto &c : grid_geom->colors()){ c = gl::COLOR_WHITE; }
            
            auto grid_mat = gl::Material::create(m_shader_warp_vert);
            grid_mat->set_depth_test(false);
            grid_mat->set_depth_write(false);
            m_grid_mesh = gl::Mesh::create(grid_geom, grid_mat);
        }
    };
    
    QuadWarp::QuadWarp()
    :m_impl(new Impl)
    {
//        set_num_subdivisions(2, 1);
    }
    
    void QuadWarp::render_output(const gl::Texture &the_texture, const float the_brightness)
    {
        if(!the_texture){ return; }
        if(!m_impl){ m_impl.reset(new Impl); }
        
#if !defined(KINSKI_GLES)
        if(the_texture.target() == GL_TEXTURE_RECTANGLE)
        {
            m_impl->m_mesh->material()->set_shader(m_impl->m_shader_warp_vert_rect);
            m_impl->m_mesh->material()->uniform("u_texture_size", the_texture.size());
        }
        else{ m_impl->m_mesh->material()->set_shader(m_impl->m_shader_warp_vert); }
#endif
        gl::Texture roi_tex = the_texture;
        if(m_impl->m_src_area != Area_<uint32_t>())
        {
            roi_tex.set_roi(m_impl->m_src_area);
        }
        
        m_impl->m_mesh->material()->textures() = {roi_tex};
        
        auto cp = m_impl->m_control_points;
        for(auto &p : cp){ p.y = 1.f - p.y; }
        
        m_impl->m_mesh->material()->uniform("u_control_points", cp);
        m_impl->m_mesh->material()->uniform("u_num_subdivisions", m_impl->m_num_subdivisions);
        m_impl->m_mesh->material()->set_diffuse(gl::Color(the_brightness, the_brightness,
                                                          the_brightness, 1.f));
        
        gl::ScopedMatrixPush model(MODEL_VIEW_MATRIX), projection(PROJECTION_MATRIX);
        gl::set_matrices(m_impl->m_camera);
        gl::draw_mesh(m_impl->m_mesh);
    }
    
    void QuadWarp::render_control_points()
    {
        if(!m_impl){ m_impl.reset(new Impl); }
        
        for(uint32_t i = 0; i < m_impl->m_control_points.size(); i++)
        {
            bool is_corner =
                !((i % (m_impl->m_num_subdivisions.x + 1)) % (m_impl->m_num_subdivisions.x)) &&
                !((i / (m_impl->m_num_subdivisions.x + 1)) % m_impl->m_num_subdivisions.y);
            
            gl::draw_circle(m_impl->m_control_points[i] * gl::window_dimension(),
                            is_corner ? 15.f : 10.f,
                            contains(m_impl->m_selected_indices, i) ?
                                gl::COLOR_RED : is_corner ? gl::COLOR_WHITE : gl::COLOR_GRAY,
                            true);
        }
    }
    
    const Area_<uint32_t>& QuadWarp::src_area() const
    {
        return m_impl->m_src_area;
    }
    
    void QuadWarp::set_src_area(const Area_<uint32_t> &the_src_area)
    {
        m_impl->m_src_area = the_src_area;
    }
    
    void QuadWarp::move_center_to(const gl::vec2 &the_pos)
    {
        auto diff = the_pos - center();
        
        for(auto &cp : m_impl->m_control_points){ cp += diff; }
    }
    
    gl::vec2 QuadWarp::center() const
    {
        return kinski::mean<gl::vec2>(m_impl->m_control_points);
    }
    
    void QuadWarp::render_grid()
    {
        if(!m_impl){ m_impl.reset(new Impl); }
        auto cp = m_impl->m_control_points;
        for(auto &p : cp){ p.y = 1.f - p.y; }
        m_impl->m_grid_mesh->material()->uniform("u_control_points", cp);
        m_impl->m_grid_mesh->material()->uniform("u_num_subdivisions", m_impl->m_num_subdivisions);
        
        gl::ScopedMatrixPush model(MODEL_VIEW_MATRIX), projection(PROJECTION_MATRIX);
        gl::set_matrices(m_impl->m_camera);
        gl::draw_mesh(m_impl->m_grid_mesh);
    }
    
    ivec2 QuadWarp::grid_resolution() const
    {
        return m_impl ? ivec2(m_impl->m_grid_num_w, m_impl->m_grid_num_h) : ivec2();
    }
    
    void QuadWarp::set_grid_resolution(const gl::ivec2 &the_res)
    {
        set_grid_resolution(the_res.x, the_res.y);
    }
    
    void QuadWarp::set_grid_resolution(uint32_t the_res_w, uint32_t the_res_h)
    {
//        auto new_impl = std::make_shared<Impl>(the_res_w, the_res_h);
//
//        if(m_impl)
//        {
//            new_impl->m_control_points = m_impl->m_control_points;
//            new_impl->m_selected_indices = m_impl->m_selected_indices;
//        }
//        m_impl = new_impl;
    }
    
    std::vector<gl::vec2>& QuadWarp::control_points()
    {
        return m_impl->m_control_points;
    }
    
    void QuadWarp::set_control_points(const std::vector<gl::vec2> &cp)
    {
        m_impl->m_control_points = cp;
    }
    
    gl::vec2& QuadWarp::control_point(int the_x, int the_y)
    {
        if(!m_impl){ m_impl.reset(new Impl); }
        the_x = clamp<int>(the_x, 0, m_impl->m_num_subdivisions.x);
        the_y = clamp<int>(the_y, 0, m_impl->m_num_subdivisions.y);
        return m_impl->m_control_points[the_x + (m_impl->m_num_subdivisions.x + 1) * the_y];
    }
    
    std::set<uint32_t>& QuadWarp::selected_indices()
    {
        return m_impl->m_selected_indices;
    }
    
    void QuadWarp::set_control_point(int the_x, int the_y, const gl::vec2 &the_point)
    {
        control_point(the_x, the_y) = the_point;
    }
    
    void QuadWarp::set_control_point(int the_index, const gl::vec2 &the_point)
    {
        // is corner ?
        bool is_corner =
            !((the_index % (m_impl->m_num_subdivisions.x + 1)) % (m_impl->m_num_subdivisions.x)) &&
            !((the_index / (m_impl->m_num_subdivisions.x + 1)) % m_impl->m_num_subdivisions.y);
        
        if(is_corner)
        {
        
        }
    }
    
    void QuadWarp::set_num_subdivisions(const gl::ivec2 &the_res)
    {
        auto num_subs = glm::min(the_res, s_max_num_subdivisions);
        auto diff = num_subs - m_impl->m_num_subdivisions;
        
        
        std::vector<gl::vec2> cp;
        int num_x = num_subs.x + 1;
        int num_y = num_subs.y + 1;
        
        // create an even grid of controlpoints
        if(m_impl->m_control_points.empty())
        {
            std::vector<gl::vec2> cp;
            gl::vec2 inc = 1.f / vec2(num_subs);
            
            for(int y = 0; y < num_y; ++y)
            {
                for(int x = 0; x < num_x; ++x)
                {
                    cp.push_back(inc * vec2(x, y));
                }
            }
            m_impl->m_control_points = cp;
        }
        else if(diff != ivec2(0))
        {
            m_impl->m_selected_indices.clear();
            
            // keep corners, recalculate subs inbetween
            auto is_corner = [this](int the_index) -> bool
            {
                return
                !((the_index % (m_impl->m_num_subdivisions.x + 1)) % (m_impl->m_num_subdivisions.x)) &&
                !((the_index / (m_impl->m_num_subdivisions.x + 1)) % m_impl->m_num_subdivisions.y);
            };
            
            std::vector<gl::vec2> corners;
            
            for(uint32_t i = 0; i < m_impl->m_control_points.size(); ++i)
            {
                if(is_corner(i)){ corners.push_back(m_impl->m_control_points[i]); }
            }
            
            // create subs for each row
            for(int y = 0; y < num_y; ++y)
            {
                float frac_y = (float)y / num_subs.y;
                auto left_edge = mix(corners[0], corners[2], frac_y);
                auto right_edge = mix(corners[1], corners[3], frac_y);
                
                for(int x = 0; x < num_x; ++x)
                {
                    float frac_x = (float)x / num_subs.x;
                    cp.push_back(mix(left_edge, right_edge, frac_x));
                }
            }
            m_impl->m_control_points = cp;
        }
        m_impl->m_num_subdivisions = num_subs;
    }
    
    void QuadWarp::set_num_subdivisions(uint32_t the_div_w, uint32_t the_div_h)
    {
        set_num_subdivisions(ivec2(the_div_w, the_div_h));
    }
    
    ivec2 QuadWarp::num_subdivisions() const
    {
        return m_impl->m_num_subdivisions;
    }
    
    void QuadWarp::reset()
    {
        m_impl.reset(new Impl);
    }
}}
