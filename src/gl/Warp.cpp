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

#include "Warp.hpp"
#include "gl/Mesh.hpp"
#include "gl/Camera.hpp"
#include "gl/BSpline.hpp"

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
    
    const gl::ivec2 Warp::s_max_num_subdivisions = gl::ivec2(7);
    
    namespace
    {
        const std::vector<gl::vec2> default_points = {gl::vec2(0, 1), gl::vec2(1, 1), gl::vec2(0, 0),
            gl::vec2(1, 0)};
        
        const Area_<float> default_roi = Area_<float>(0.f, 0.f, 1.f, 1.f);
    };
    
    struct WarpImpl
    {
        uint32_t m_grid_num_w = 32, m_grid_num_h = 18;
        
        ivec2 m_num_subdivisions = ivec2(1, 1);
        
        std::vector<gl::vec2> m_corners = default_points;
        std::vector<gl::vec2> m_control_points = default_points;
        std::set<uint32_t> m_selected_indices;
        
        gl::OrthographicCamera::Ptr
        m_camera = gl::OrthographicCamera::create(0, 1, 0, 1, -100, 100);
        
        gl::MeshPtr m_mesh, m_grid_mesh;
        
        Area_<float> m_src_area = default_roi;
        
        mat4 m_transform, m_inv_transform;
        bool m_dirty = true, m_dirty_subs = true, m_perspective = true;
        bool m_cubic_interpolation = true;

        gl::vec4 m_edges = gl::vec4(0, 1, 1, 0), m_edge_exponents = gl::vec4(1);
        
        WarpImpl()
        {
            create_mesh(m_grid_num_w, m_grid_num_h);
        }
        
        inline bool is_corner(int the_index)
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
            geom->compute_aabb();
            
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
            auto &colors = m_mesh->geometry()->colors();
            
            std::vector<vec2> cols, rows;
            
            for(uint32_t y = 0; y < m_grid_num_h + 1; ++y)
            {
                for(uint32_t x = 0; x < m_grid_num_w + 1; ++x, ++index)
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
                                cols.push_back(control_point(col + i, row + j));
                            }
                            rows.push_back(cubic_interpolate(cols, v));
                        }
                        p = cubic_interpolate(rows, u);
                    }
                    verts[index] = vec3(p, 0);

                    //softedges
                    float alpha = 1.f;
                    float edge_u = x / (float)(m_grid_num_w);
                    float edge_v = y / (float)(m_grid_num_h);

                    if(m_edges.x > 0){ alpha *= std::pow(clamp(edge_v / m_edges.x, 0.f, 1.f), m_edge_exponents.x); }
                    if(m_edges.z < 1.f){ alpha *= std::pow(map_value(edge_v, 1.f, m_edges.z, 0.f, 1.f), m_edge_exponents.z); }
                    if(m_edges.w > 0){ alpha *= std::pow(clamp(edge_u / m_edges.w, 0.f, 1.f), m_edge_exponents.w); }
                    if(m_edges.y < 1.f){ alpha *= std::pow(map_value(edge_u, 1.f, m_edges.y, 0.f, 1.f), m_edge_exponents.y); }

                    colors[index].a = alpha;
                }
            }
            //TODO
//            m_mesh->geometry()->create_gl_buffers(GL_DYNAMIC_DRAW);
            m_dirty_subs = false;
            
            // recreate grid mesh
            auto grid_geom = gl::Geometry::create();
            grid_geom->append_vertices(m_mesh->geometry()->vertices());
            grid_geom->set_primitive_type(GL_LINES);
            grid_geom->colors().resize(m_mesh->geometry()->vertices().size(), gl::COLOR_WHITE);
            auto &indices = grid_geom->indices();
            
            uint32_t num_w = m_grid_num_w + 1, num_h = m_grid_num_h + 1;
            
            for(uint32_t y = 0; y < num_h; ++y)
            {
                for(uint32_t x = 0; x < num_w; ++x)
                {
                    uint32_t index = x + y * num_w;
                    
                    if(x < m_grid_num_w)
                    {
                        indices.push_back(index);
                        indices.push_back(index + 1);
                    }
                    
                    if(y < m_grid_num_h)
                    {
                        indices.push_back(index);
                        indices.push_back(index + num_w);
                    }
                }
            }
            
            auto grid_mat = gl::Material::create();
            grid_mat->set_depth_test(false);
            grid_mat->set_depth_write(false);
            m_grid_mesh = gl::Mesh::create(grid_geom, grid_mat);
        }
        
        void set_num_subdivisions_x(uint32_t the_num_subs_x)
        {
            m_selected_indices.clear();
            
            uint32_t num_controls_x = m_num_subdivisions.x + 1;
            uint32_t num_controls_y = m_num_subdivisions.y + 1;
            uint32_t new_num_x = the_num_subs_x + 1;
            std::vector<gl::vec2> cp(new_num_x * num_controls_y);
            
            // spline fitting
            for(uint32_t row = 0; row < num_controls_y; ++row)
            {
                std::vector<vec2> points;

                if(!m_cubic_interpolation)
                {
                    // construct piece-wise linear spline
                    for(uint32_t col = 0; col < num_controls_x; ++col)
                    {
                        points.push_back(control_point(col, row));
                    }
                    BSpline2f s(points, 1, false, true);

                    // calculate position of new control points
                    float length = s.length(0.0f, 1.0f);
                    float step = 1.0f / (new_num_x - 1);

                    for(uint32_t col = 0; col < new_num_x; ++col)
                    {
                        cp[row * new_num_x + col] = s.position(s.time(length * col * step));
                    }
                }
                else
                {
                    // construct piece-wise catmull-rom spline
                    for(uint32_t col = 0; col < num_controls_x; ++col)
                    {
                        vec2 p0 = control_point(col - 1, row);
                        vec2 p1 = control_point(col, row);
                        vec2 p2 = control_point(col + 1, row);
                        vec2 p3 = control_point(col + 2, row);

                        // control points according to an optimized Catmull-Rom implementation
                        vec2 b1 = p1 + (p2 - p0) / 6.0f;
                        vec2 b2 = p2 - (p3 - p1) / 6.0f;
                        points.push_back(p1);

                        if(col < num_controls_x - 1)
                        {
                            points.push_back(b1);
                            points.push_back(b2);
                        }
                    }
                    BSpline2f s(points, 3, false, true);

                    // calculate position of new control points
                    float length = s.length(0.0f, 1.0f);
                    float step = 1.0f / (new_num_x - 1);

                    for(uint32_t col = 0; col < new_num_x; ++col)
                    {
                        cp[row * new_num_x + col] = s.position(s.time(length * col * step));
                    }
                }
            }
            m_control_points = cp;
            m_num_subdivisions.x = the_num_subs_x;
            m_dirty_subs = true;
        }
        
        void set_num_subdivisions_y(uint32_t the_num_subs_y)
        {
            m_selected_indices.clear();
            uint32_t num_controls_x = m_num_subdivisions.x + 1;
            uint32_t num_controls_y = m_num_subdivisions.y + 1;
            uint32_t new_num_y = the_num_subs_y + 1;
            std::vector<gl::vec2> cp(num_controls_x * new_num_y);
            
            // spline fitting
            for(uint32_t col = 0; col < num_controls_x; ++col)
            {
                std::vector<vec2> points;
                
                if(!m_cubic_interpolation)
                {
                    // construct piece-wise linear spline
                    for(uint32_t row = 0; row < num_controls_y; ++row)
                    {
                        points.push_back(control_point(col, row));
                    }
                    BSpline2f s(points, 1, false, true);
                    
                    // calculate position of new control points
                    float length = s.length(0.0f, 1.0f);
                    float step = 1.0f / (new_num_y - 1);
                    
                    for(uint32_t row = 0; row < new_num_y; ++row)
                    {
                        cp[row * num_controls_x + col] = s.position(s.time(length * row * step));
                    }
                }
                else
                {
                    // construct piece-wise catmull-rom spline
                    for(uint32_t row = 0; row < num_controls_y; ++row)
                    {
                        vec2 p0 = control_point(col, row - 1);
                        vec2 p1 = control_point(col, row);
                        vec2 p2 = control_point(col, row + 1);
                        vec2 p3 = control_point(col, row + 2);
                        
                        // control points according to an optimized Catmull-Rom implementation
                        vec2 b1 = p1 + (p2 - p0) / 6.0f;
                        vec2 b2 = p2 - (p3 - p1) / 6.0f;
                        points.push_back(p1);
                        
                        if(row < num_controls_y - 1)
                        {
                            points.push_back(b1);
                            points.push_back(b2);
                        }
                    }
                    BSpline2f s(points, 3, false, true);
                    
                    // calculate position of new control points
                    float length = s.length(0.0f, 1.0f);
                    float step = 1.0f / (new_num_y - 1);
                    
                    for(uint32_t row = 0; row < new_num_y; ++row)
                    {
                        cp[row * num_controls_x + col] = s.position(s.time(length * row * step));
                    }
                }
            }
            m_control_points = cp;
            m_num_subdivisions.y = the_num_subs_y;
            m_dirty_subs = true;
        }
        
        const gl::vec2 control_point(int the_x, int the_y)
        {
            // extrapolate points beyond the edges
            if(the_x < 0){ return 2.0f * control_point(0, the_y) - control_point(-the_x, the_y); }
            
            if(the_x > m_num_subdivisions.x)
            {
                return 2.0f * control_point(m_num_subdivisions.x, the_y) -
                       control_point(2 * m_num_subdivisions.x - the_x, the_y);
            }
            
            if(the_y < 0){ return 2.0f * control_point(the_x, 0) - control_point(the_x, -the_y); }
            
            if(the_y > m_num_subdivisions.y)
            {
                return 2.0f * control_point(the_x, m_num_subdivisions.y) -
                       control_point(the_x, 2 * m_num_subdivisions.y - the_y);
            }
            return m_control_points[the_x + (m_num_subdivisions.x + 1) * the_y];
        }
        
        int convert_index(uint32_t the_index)
        {
            uint32_t index_tl = 0, index_tr = m_num_subdivisions.x;
            uint32_t index_bl = (m_num_subdivisions.x + 1) * m_num_subdivisions.y;
            uint32_t index_br = index_bl + m_num_subdivisions.x;
            
            if(the_index == index_tl) return 0;
            if(the_index == index_tr) return 1;
            if(the_index == index_bl) return 2;
            if(the_index == index_br) return 3;
            return -1;
        }
    };
    
    Warp::Warp(): m_impl(new WarpImpl){}
    
    void Warp::render_output(const gl::Texture &the_texture, const float the_brightness)
    {
        if(m_impl->m_dirty_subs){ m_impl->update_mesh(); }
        if(!the_texture){ return; }
        
#if !defined(KINSKI_GLES)
        if(the_texture.target() == GL_TEXTURE_RECTANGLE)
        {
            m_impl->m_mesh->material()->set_shader(gl::create_shader(gl::ShaderType::RECT_2D));
            m_impl->m_mesh->material()->uniform("u_texture_size", the_texture.size());
        }
        else{ m_impl->m_mesh->material()->set_shader(gl::create_shader(gl::ShaderType::UNLIT)); }

#endif
        gl::Texture roi_tex = the_texture;
        if(m_impl->m_src_area != default_roi)
        {
            uvec2 sz(the_texture.width() - 1, the_texture.height() - 1);
            
            Area_<uint32_t> abs_roi(m_impl->m_src_area.x0 * sz.x, m_impl->m_src_area.y0 * sz.y,
                                    m_impl->m_src_area.x1 * sz.x, m_impl->m_src_area.y1 * sz.y);
            roi_tex.set_roi(abs_roi);
        }
        
        m_impl->m_mesh->material()->set_textures({roi_tex});
        
        m_impl->m_mesh->material()->set_diffuse(gl::Color(the_brightness, the_brightness,
                                                          the_brightness, 1.f));
        
        gl::ScopedMatrixPush model(MODEL_VIEW_MATRIX), projection(PROJECTION_MATRIX);
        gl::set_matrices(m_impl->m_camera);
        gl::mult_matrix(MODEL_VIEW_MATRIX, transform());
        gl::draw_mesh(m_impl->m_mesh);
    }
    
    void Warp::render_control_points()
    {
        for(uint32_t i = 0; i < m_impl->m_control_points.size(); i++)
        {
            bool is_corner = m_impl->is_corner(i);
            
            vec2 p = control_point(i);
            p.y = (1.f - p.y);
            
            float sz = is_corner ? 14.f : 9.f;
            auto col = contains(m_impl->m_selected_indices, i) ? gl::COLOR_RED : is_corner ?
                gl::COLOR_WHITE : gl::COLOR_GRAY;
            
            gl::draw_circle(p * window_dimension(), sz / 4.f, col, true, 24);
            gl::draw_circle(p * window_dimension(), sz, col, false, 24);
        }
    }
    
    const Area_<float>& Warp::src_area() const
    {
        return m_impl->m_src_area;
    }
    
    void Warp::set_src_area(const Area_<float> &the_src_area)
    {
        m_impl->m_src_area = the_src_area;
    }
    
    void Warp::move_center_to(const gl::vec2 &the_pos)
    {
        auto diff = the_pos - center();
        
        if(m_impl->m_perspective){ for(auto &cp : m_impl->m_corners){ cp += diff; } }
        else{ for(auto &cp : m_impl->m_control_points){ cp += diff; } }
    }
    
    gl::vec2 Warp::center() const
    {
        if(m_impl->m_perspective){ return kinski::mean<gl::vec2>(m_impl->m_corners); }
        else{ return kinski::mean<gl::vec2>(m_impl->m_control_points); }
    }
    
    void Warp::flip_content(bool horizontal)
    {
        if(horizontal)
        {
            std::swap(m_impl->m_corners[0], m_impl->m_corners[2]);
            std::swap(m_impl->m_corners[1], m_impl->m_corners[3]);
        }
        else
        {
            std::swap(m_impl->m_corners[0], m_impl->m_corners[1]);
            std::swap(m_impl->m_corners[2], m_impl->m_corners[3]);
        }
        
        // adjust culling
        if(m_impl->m_mesh->material()->culling() == gl::Material::CULL_BACK)
        {
            m_impl->m_mesh->material()->set_culling(gl::Material::CULL_FRONT);
        }else{ m_impl->m_mesh->material()->set_culling(gl::Material::CULL_BACK); }
    }
    
    void Warp::rotate_content(bool clock_wise)
    {
        if(clock_wise)
        {
            std::swap(m_impl->m_corners[0], m_impl->m_corners[2]);
            std::swap(m_impl->m_corners[1], m_impl->m_corners[3]);
            std::swap(m_impl->m_corners[0], m_impl->m_corners[3]);
        }
        else
        {
            std::swap(m_impl->m_corners[0], m_impl->m_corners[1]);
            std::swap(m_impl->m_corners[2], m_impl->m_corners[3]);
            std::swap(m_impl->m_corners[0], m_impl->m_corners[3]);
        }
        // recreate mesh to swap subdivisions
        auto culling = m_impl->m_mesh->material()->culling();
        m_impl->create_mesh(m_impl->m_grid_num_h, m_impl->m_grid_num_w);
        m_impl->m_mesh->material()->set_culling(culling);
    }
    
    void Warp::render_grid()
    {
        gl::ScopedMatrixPush model(MODEL_VIEW_MATRIX), projection(PROJECTION_MATRIX);
        gl::set_matrices(m_impl->m_camera);
        gl::mult_matrix(MODEL_VIEW_MATRIX, transform());
        gl::draw_mesh(m_impl->m_grid_mesh);
    }
    
    void Warp::render_boundary(const gl::Color &the_colour)
    {
        gl::ScopedMatrixPush model(MODEL_VIEW_MATRIX), projection(PROJECTION_MATRIX);
        gl::set_matrices(m_impl->m_camera);
        gl::mult_matrix(MODEL_VIEW_MATRIX, transform());
        
        std::vector<gl::vec3> points =
        {
            gl::vec3(default_points[0], 0.f), gl::vec3(default_points[1], 0.f),
            gl::vec3(default_points[3], 0.f), gl::vec3(default_points[2], 0.f),
            gl::vec3(default_points[0], 0.f)
        };
        gl::draw_linestrip(points, the_colour);
    }
    
    ivec2 Warp::grid_resolution() const
    {
        return ivec2(m_impl->m_grid_num_w, m_impl->m_grid_num_h);
    }

    void Warp::set_grid_resolution(const gl::ivec2 &the_res)
    {
        set_grid_resolution(the_res.x, the_res.y);
    }

    void Warp::set_grid_resolution(uint32_t the_res_w, uint32_t the_res_h)
    {
        if(m_impl->m_grid_num_w != the_res_w || m_impl->m_grid_num_h != the_res_h)
        {
            m_impl->create_mesh(the_res_w, the_res_h);
        }
    }
    
    const std::vector<gl::vec2>& Warp::control_points() const
    {
        return m_impl->m_control_points;
    }
    
    void Warp::set_control_points(const std::vector<gl::vec2> &cp)
    {
        m_impl->m_control_points = cp;
        m_impl->m_dirty = true;
        m_impl->m_dirty_subs = true;
    }
    
    const gl::vec2 Warp::control_point(int the_x, int the_y) const
    {
        return m_impl->control_point(the_x, the_y);
    }
    
    const gl::vec2 Warp::control_point(int the_index) const
    {
        auto tmp = transform() * vec4(m_impl->m_control_points[the_index], 0, 1);
        return (tmp / tmp.w).xy();
    }
    
    std::set<uint32_t>& Warp::selected_indices()
    {
        return m_impl->m_selected_indices;
    }
    
    void Warp::set_control_point(int the_x, int the_y, const gl::vec2 &the_point)
    {
        set_control_point(the_x + (m_impl->m_num_subdivisions.x + 1) * the_y, the_point);
    }
    
    void Warp::set_control_point(int the_index, const gl::vec2 &the_point)
    {
        the_index = clamp<int>(the_index, 0 , m_impl->m_control_points.size() - 1);
        
        if(m_impl->m_perspective && m_impl->is_corner(the_index))
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
    
    void Warp::set_num_subdivisions(const gl::ivec2 &the_res)
    {
        auto num_subs = glm::clamp(the_res, gl::ivec2(1), Warp::s_max_num_subdivisions);
        auto diff = num_subs - m_impl->m_num_subdivisions;
        if(diff.x){ m_impl->set_num_subdivisions_x(num_subs.x); }
        if(diff.y){ m_impl->set_num_subdivisions_y(num_subs.y); }
    }
    
    void Warp::set_num_subdivisions(uint32_t the_div_w, uint32_t the_div_h)
    {
        set_num_subdivisions(ivec2(the_div_w, the_div_h));
    }
    
    gl::vec4 Warp::edges() const
    {
        gl::vec4 ret = m_impl->m_edges;
        ret.y = 1.f - ret.y;
        ret.z = 1.f - ret.z;
        return ret;
    }
    
    gl::vec4 Warp::edge_exponents() const
    {
        return m_impl->m_edge_exponents;
    }
    
    void Warp::set_edges(const gl::vec4& the_factors)
    {
        m_impl->m_edges.x = clamp(the_factors.x, 0.f, 1.f);
        m_impl->m_edges.y = clamp(1.f - the_factors.y, 0.f, 1.f);
        m_impl->m_edges.z = clamp(1.f - the_factors.z, 0.f, 1.f);
        m_impl->m_edges.w = clamp(the_factors.w, 0.f, 1.f);
        m_impl->m_dirty_subs = true;
    }

    void Warp::set_edge_exponents(const gl::vec4& the_exponents)
    {
        m_impl->m_edge_exponents = the_exponents;
        m_impl->m_dirty_subs = true;
    }

    ivec2 Warp::num_subdivisions() const
    {
        return m_impl->m_num_subdivisions;
    }
    
    void Warp::reset()
    {
        m_impl.reset(new WarpImpl);
        set_num_subdivisions(num_subdivisions());
    }
    
    const std::vector<gl::vec2>& Warp::corners() const
    {
        return m_impl->m_corners;
    }
    
    void Warp::set_corners(const std::vector<gl::vec2> &cp)
    {
        if(cp.size() == 4)
        {
            m_impl->m_corners = cp;
            m_impl->m_dirty = true;
        }
    }
    
    const gl::mat4& Warp::transform() const
    {
        if(m_impl->m_dirty)
        {
            if(m_impl->m_perspective)
            {
                auto src = default_points;
                auto dest = m_impl->m_corners; //corners();
                std::swap(src[2], src[3]);
                std::swap(dest[2], dest[3]);
                m_impl->m_transform = calculate_homography(src.data(), dest.data());
                m_impl->m_inv_transform = glm::inverse(m_impl->m_transform);
            }
            else{ m_impl->m_transform = m_impl->m_inv_transform = glm::mat4(); }
            m_impl->m_dirty = false;
        }
        return m_impl->m_transform;
    }
    
    bool Warp::perspective() const
    {
        return m_impl->m_perspective;
    }
    
    void Warp::set_perspective(bool b)
    {
        if(b != m_impl->m_perspective)
        {
            m_impl->m_perspective = b;
            m_impl->m_dirty = m_impl->m_dirty_subs = true;
        }
    }
    
    void Warp::set_cubic_interpolation(bool b)
    {
        m_impl->m_cubic_interpolation = b;
        m_impl->m_dirty_subs = true;
    }
    
    bool Warp::cubic_interpolation() const
    {
        return m_impl->m_cubic_interpolation;
    }
    
}}
