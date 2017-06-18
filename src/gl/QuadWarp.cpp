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
    
    ///////////////////////////////// utilities ////////////////////////////////////////////////////
    
    vec2 cubic_interpolate(const std::vector<vec2> &knots, float t)
    {
        assert( knots.size() >= 4 );
        
        return knots[1] + 0.5f * t * (knots[2] - knots[0] +
                                      t * (2.0f * knots[0] - 5.0f * knots[1] + 4.0f * knots[2] -
                                           knots[3] + t * (3.0f * (knots[1] - knots[2]) +
                                                           knots[3] - knots[0])));
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
    const gl::ivec2 QuadWarp::s_max_num_subdivisions = gl::ivec2(5);
    
    namespace
    {
        const std::vector<gl::vec2> default_points = {gl::vec2(0, 1), gl::vec2(1, 1), gl::vec2(0, 0),
            gl::vec2(1, 0)};
    };
    
    struct Impl
    {
        uint32_t m_grid_num_w = 64, m_grid_num_h = 36;
        
        ivec2 m_num_subdivisions = ivec2(1, 1);
        
        std::vector<gl::vec2> m_corners = default_points;
        std::vector<gl::vec2> m_control_points = default_points;
        std::set<uint32_t> m_selected_indices;
        
        gl::OrthographicCamera::Ptr
        m_camera = gl::OrthographicCamera::create(0, 1, 0, 1, 0, 1);
        
        gl::MeshPtr m_mesh, m_grid_mesh;
        
        Area_<uint32_t> m_src_area;
        
        mat4 m_transform, m_inv_transform;
        bool m_dirty = true, m_dirty_subs = true;
        bool m_cubic_interpolation = false;
        
        Impl(uint32_t the_res_w = 32, uint32_t the_res_h = 18):
        m_grid_num_w(the_res_w), m_grid_num_h(the_res_h)
        {
            create_mesh(the_res_w, the_res_h);
            
            auto grid_geom = gl::Geometry::create_grid(1.f, 1.f, m_grid_num_w, m_grid_num_h);
            for(auto &v : grid_geom->vertices())
            {
                v = vec3(v.x, -v.z, v.y) + vec3(0.5f, 0.5f, 0.f);
            }
            for(auto &c : grid_geom->colors()){ c = gl::COLOR_WHITE; }
            
            auto grid_mat = gl::Material::create();
            grid_mat->set_depth_test(false);
            grid_mat->set_depth_write(false);
            m_grid_mesh = gl::Mesh::create(grid_geom, grid_mat);
        }
        
        inline bool is_corner(uint32_t the_index)
        {
            int num_cp = (m_num_subdivisions.x + 1) * (m_num_subdivisions.y + 1);
            return  the_index == 0 ||
                    the_index == m_num_subdivisions.x ||
                    the_index == (num_cp - m_num_subdivisions.x - 1) ||
                    the_index == (num_cp - 1);
        }
        
        void create_mesh(uint32_t the_res_w, uint32_t the_res_h)
        {
            m_grid_num_w = the_res_w;
            m_grid_num_h = the_res_h;
            
            // adjust grid density here
            auto geom = gl::Geometry::create_plane(1, 1, m_grid_num_w, m_grid_num_h);
            for(auto &v : geom->vertices()){ v += vec3(0.5f, 0.5f, 0.f); }
            geom->compute_bounding_box();
            
            auto mat = gl::Material::create();
            mat->set_depth_test(false);
            mat->set_depth_write(false);
            mat->set_blending(true);
            m_mesh = gl::Mesh::create(geom, mat);
            m_dirty_subs = true;
        }
        
        void update_mesh()
        {
            vec2 p;
            float u, v;
            int col, row;
            
            uint32_t index = 0;
            auto &verts = m_mesh->geometry()->vertices();
            
            std::vector<vec2> cols, rows;
            
            for(int y = 0; y < m_grid_num_h + 1; ++y)
            {
                for(int x = 0; x < m_grid_num_w + 1; ++x)
                {
                    // transform coordinates to [0..numControls]
                    u = x * (m_num_subdivisions.x) / (float)(m_grid_num_w);
                    v = y * (m_num_subdivisions.y) / (float)(m_grid_num_h);
                    
                    // determine col and row
                    col = (int)(u);
                    row = (int)(v);
                    
                    // normalize coordinates to [0..1]
                    u -= col;
                    v -= row;
                    
                    if(!m_cubic_interpolation)
                    {
                        // perform linear interpolation
                        vec2 p1 = mix(control_point(col, row), control_point(col + 1, row), u);
                        vec2 p2 = mix(control_point(col, row + 1), control_point(col + 1, row + 1), u);
                        p = mix(p1, p2, v);
                    }
                    else
                    {
                        // perform bicubic interpolation
                        rows.clear();
                        for(int i = -1; i < 3; ++i)
                        {
                            cols.clear();
                            for(int j = -1; j < 3; ++j)
                            {
                                cols.push_back( control_point(col + i, row + j));
                            }
                            rows.push_back(cubic_interpolate(cols, v));
                        }
                        p = cubic_interpolate(rows, u);
                    }
                    verts[index++] = vec3(p, 0);
                }
            }
            m_mesh->geometry()->create_gl_buffers();
            m_dirty_subs = false;
        }
        
        void set_num_subdivisions(const gl::ivec2 &the_res)
        {
//            auto num_subs = glm::min(the_res, QuadWarp::s_max_num_subdivisions);
            auto num_subs = glm::clamp(the_res, gl::ivec2(1), QuadWarp::s_max_num_subdivisions);
            auto diff = num_subs - m_num_subdivisions;
            
            
            std::vector<gl::vec2> cp;
            int num_x = num_subs.x + 1;
            int num_y = num_subs.y + 1;
            
            // create an even grid of controlpoints
            if(m_control_points.empty())
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
                m_control_points = cp;
            }
            else if(diff != ivec2(0))
            {
                m_selected_indices.clear();
                
                // create subs for each row
                for(int y = 0; y < num_y; ++y)
                {
                    float frac_y = (float)y / num_subs.y;
                    auto left_edge = mix(default_points[0], default_points[2], frac_y);
                    auto right_edge = mix(default_points[1], default_points[3], frac_y);
                    
                    for(int x = 0; x < num_x; ++x)
                    {
                        float frac_x = (float)x / num_subs.x;
                        cp.push_back(mix(left_edge, right_edge, frac_x));
                    }
                }
                m_control_points = cp;
                m_num_subdivisions = num_subs;
                m_dirty_subs = true;
            }
            m_num_subdivisions = num_subs;
        }
        
        inline gl::vec2& control_point(int the_x, int the_y)
        {
            the_x = clamp<int>(the_x, 0, m_num_subdivisions.x);
            the_y = clamp<int>(the_y, 0, m_num_subdivisions.y);
            return m_control_points[the_x + (m_num_subdivisions.x + 1) * the_y];
        }
        
        int convert_index(uint32_t the_index)
        {
            int index_tl = 0, index_tr = m_num_subdivisions.x;
            int index_bl = (m_num_subdivisions.x + 1) * m_num_subdivisions.y;
            int index_br = index_bl + m_num_subdivisions.x;
            
            if(the_index == index_tl) return 0;
            if(the_index == index_tr) return 1;
            if(the_index == index_bl) return 2;
            if(the_index == index_br) return 3;
            return -1;
        }
    };
    
    QuadWarp::QuadWarp()
    :m_impl(new Impl)
    {

    }
    
    void QuadWarp::render_output(const gl::Texture &the_texture, const float the_brightness)
    {
        if(!the_texture){ return; }
        if(!m_impl){ m_impl.reset(new Impl); }
        
#if !defined(KINSKI_GLES)
        if(the_texture.target() == GL_TEXTURE_RECTANGLE)
        {
            m_impl->m_mesh->material()->set_shader(gl::create_shader(gl::ShaderType::RECT_2D));
            m_impl->m_mesh->material()->uniform("u_texture_size", the_texture.size());
        }
        else{ m_impl->m_mesh->material()->set_shader(gl::create_shader(gl::ShaderType::UNLIT)); }

#endif
        gl::Texture roi_tex = the_texture;
        if(m_impl->m_src_area != Area_<uint32_t>())
        {
            roi_tex.set_roi(m_impl->m_src_area);
        }
        
        m_impl->m_mesh->material()->textures() = {roi_tex};
        
        if(m_impl->m_dirty_subs){ m_impl->update_mesh(); }
        
        m_impl->m_mesh->material()->set_diffuse(gl::Color(the_brightness, the_brightness,
                                                          the_brightness, 1.f));
        
        gl::ScopedMatrixPush model(MODEL_VIEW_MATRIX), projection(PROJECTION_MATRIX);
        gl::set_matrices(m_impl->m_camera);
        gl::mult_matrix(MODEL_VIEW_MATRIX, transform());
        gl::draw_mesh(m_impl->m_mesh);
    }
    
    void QuadWarp::render_control_points()
    {
        if(!m_impl){ m_impl.reset(new Impl); }
        
        for(uint32_t i = 0; i < m_impl->m_control_points.size(); i++)
        {
            bool is_corner = m_impl->is_corner(i);
            
            vec2 p = control_point(i);
            p.y = (1.f - p.y);
            
            gl::draw_circle(p * window_dimension(),
                            is_corner ? 15.f : 10.f,
                            contains(m_impl->m_selected_indices, i) ?
                                gl::COLOR_RED : is_corner ? gl::COLOR_WHITE : gl::COLOR_GRAY,
                            true);
        }
        
//        uint32_t num_x = m_impl->m_num_subdivisions.x + 1, num_y = m_impl->m_num_subdivisions.x + 1;
        
//        for(uint32_t y = 0; y < num_y; ++y)
//            for(uint32_t x = 0; x < num_x; ++x)
//            {
//                
//            }
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
        
        for(auto &cp : m_impl->m_corners){ cp += diff; }
    }
    
    gl::vec2 QuadWarp::center() const
    {
        return kinski::mean<gl::vec2>(m_impl->m_corners);
    }
    
    void QuadWarp::render_grid()
    {
        gl::ScopedMatrixPush model(MODEL_VIEW_MATRIX), projection(PROJECTION_MATRIX);
        gl::set_matrices(m_impl->m_camera);
        gl::mult_matrix(MODEL_VIEW_MATRIX, transform());
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
        m_impl->create_mesh(the_res_w, the_res_h);
    }
    
    const std::vector<gl::vec2>& QuadWarp::control_points() const
    {
        return m_impl->m_control_points;
    }
    
    void QuadWarp::set_control_points(const std::vector<gl::vec2> &cp)
    {
        m_impl->m_control_points = cp;
        m_impl->m_dirty = true;
        m_impl->m_dirty_subs = true;
    }
    
    gl::vec2& QuadWarp::control_point(int the_x, int the_y)
    {
        if(!m_impl){ m_impl.reset(new Impl); }
        return m_impl->control_point(the_x, the_y);
    }
    
    const gl::vec2 QuadWarp::control_point(int the_index)
    {
        auto tmp = transform() * vec4(m_impl->m_control_points[the_index], 0, 1);
        return (tmp / tmp.w).xy();
    }
    
    std::set<uint32_t>& QuadWarp::selected_indices()
    {
        return m_impl->m_selected_indices;
    }
    
    void QuadWarp::set_control_point(int the_x, int the_y, const gl::vec2 &the_point)
    {
        set_control_point(the_x + (m_impl->m_num_subdivisions.x + 1) * the_y, the_point);
    }
    
    void QuadWarp::set_control_point(int the_index, const gl::vec2 &the_point)
    {
        the_index = clamp<int>(the_index, 0 , m_impl->m_control_points.size() - 1);
        
        if(m_impl->is_corner(the_index))
        {
            m_impl->m_dirty = true;
            
            // convert to corner index
            m_impl->m_corners[m_impl->convert_index(the_index)] = the_point;
        }
        else
        {
            // convert to warp space
            auto p = m_impl->m_inv_transform * vec4(the_point, 0, 1);
            p /= p.w;
            
            m_impl->m_control_points[the_index] = p.xy();
            m_impl->m_dirty_subs = true;
        }
    }
    
    void QuadWarp::set_num_subdivisions(const gl::ivec2 &the_res)
    {
        m_impl->set_num_subdivisions(the_res);
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
        set_num_subdivisions(num_subdivisions());
    }
    
    const std::vector<gl::vec2>& QuadWarp::corners() const
    {
        return m_impl->m_corners;
    }
    
    void QuadWarp::set_corners(const std::vector<gl::vec2> &cp)
    {
        if(cp.size() == 4)
        {
            m_impl->m_corners = cp;
            m_impl->m_dirty = true;
        }
    }
    
    const gl::mat4& QuadWarp::transform() const
    {
        if(m_impl->m_dirty)
        {
            auto src = default_points;
            auto dest = m_impl->m_corners; //corners();
            std::swap(src[2], src[3]);
            std::swap(dest[2], dest[3]);
            m_impl->m_transform = calculate_homography(src.data(), dest.data());
            m_impl->m_inv_transform = glm::inverse(m_impl->m_transform);
            m_impl->m_dirty = false;
        }
        return m_impl->m_transform;
    }
    
    void QuadWarp::set_cubic_interpolation(bool b)
    {
        m_impl->m_cubic_interpolation = b;
        m_impl->m_dirty_subs = true;
    }
    
    bool QuadWarp::cubic_interpolation() const
    {
        return m_impl->m_cubic_interpolation;
    }
    
}}
