//
//  Noise.cpp
//  kinskiGL
//
//  Created by Croc Dialer on 25/08/15.
//
//

#include "QuadWarp.h"
#include "gl/Mesh.h"
#include "gl/Camera.h"
#include "gl/ShaderLibrary.h"

namespace kinski{ namespace gl{

    struct QuadWarp::Impl
    {
        uint32_t m_grid_num_w, m_grid_num_h;
        
        std::vector<gl::vec2> m_control_points;
        std::set<uint32_t> m_selected_indices;
        
        gl::OrthographicCamera::Ptr
        m_camera = gl::OrthographicCamera::create(0, 1, 0, 1, 0, 1);
        
        gl::MeshPtr m_mesh, m_grid_mesh;
        gl::MaterialPtr m_handle_material;
        
        Impl():
        m_grid_num_w(10), m_grid_num_h(10)
        {
            m_control_points = {gl::vec2(0, 0), gl::vec2(1, 0), gl::vec2(0, 1), gl::vec2(1, 1)};
            
            // adjust grid density here
            auto geom = gl::Geometry::createPlane(1, 1, m_grid_num_w, m_grid_num_h);
            for(auto &v : geom->vertices()){ v += vec3(0.5f, 0.5f, 0.f); }
            geom->computeBoundingBox();
            gl::Shader shader;
            shader.loadFromData(quad_warp_vert, unlit_frag);
            auto mat = gl::Material::create(shader);
            mat->setDepthTest(false);
            mat->setDepthWrite(false);
            m_mesh = gl::Mesh::create(geom, mat);
            
            auto grid_geom = gl::Geometry::create_grid(1.f, 1.f, m_grid_num_w, m_grid_num_h);
            for(auto &v : grid_geom->vertices())
            {
                v = vec3(v.x, -v.z, v.y) + vec3(0.5f, 0.5f, 0.f);
            }
            
            auto grid_mat = gl::Material::create(shader);
            grid_mat->setDepthTest(false);
            grid_mat->setDepthWrite(false);
            m_grid_mesh = gl::Mesh::create(grid_geom, grid_mat);
            m_handle_material = gl::Material::create();
        }
    };
    
    QuadWarp::QuadWarp()
    {

    }
    
    void QuadWarp::init()
    {
        
    }
    
    void QuadWarp::render_output(gl::Texture &the_texture)
    {
        if(!the_texture){ return; }
        if(!m_impl){ m_impl.reset(new Impl); }
        
        m_impl->m_mesh->material()->textures() = {the_texture};
        
        auto cp = m_impl->m_control_points;
        for(auto &p : cp){ p.y = 1.f - p.y; }
        m_impl->m_mesh->material()->uniform("u_control_points", cp);
        
        gl::ScopedMatrixPush model(MODEL_VIEW_MATRIX), projection(PROJECTION_MATRIX);
        gl::setMatrices(m_impl->m_camera);
        gl::drawMesh(m_impl->m_mesh);
    }
    
    void QuadWarp::render_control_points()
    {
        if(!m_impl){ m_impl.reset(new Impl); }
        
        for(int i = 0; i < m_impl->m_control_points.size(); i++)
        {
            gl::drawCircle(m_impl->m_control_points[i] * gl::windowDimension(), 20.f, false,
                           m_impl->m_handle_material);
        }
    }
    
    void QuadWarp::render_grid()
    {
        if(!m_impl){ m_impl.reset(new Impl); }
        auto cp = m_impl->m_control_points;
        for(auto &p : cp){ p.y = 1.f - p.y; }
        m_impl->m_grid_mesh->material()->uniform("u_control_points", cp);
        
        gl::ScopedMatrixPush model(MODEL_VIEW_MATRIX), projection(PROJECTION_MATRIX);
        gl::setMatrices(m_impl->m_camera);
        gl::drawMesh(m_impl->m_grid_mesh);
    }
    
    uint32_t QuadWarp::grid_num_w() const { return m_impl->m_grid_num_w; };
    uint32_t QuadWarp::grid_num_h() const { return m_impl->m_grid_num_h; };
    
    const gl::vec2& QuadWarp::control_point(int the_x, int the_y) const
    {
        return control_point(the_x, the_y);
    }
    
    gl::vec2& QuadWarp::control_point(int the_x, int the_y)
    {
        if(!m_impl){ m_impl.reset(new Impl); }
        the_x = clamp(the_x, 0, 1);
        the_y = clamp(the_y, 0, 1);
        return m_impl->m_control_points[the_x + 2 * the_y];
    }
    
    void QuadWarp::set_control_point(int the_x, int the_y, const gl::vec2 &the_point)
    {
        control_point(the_x, the_y) = the_point;
    }
}}
