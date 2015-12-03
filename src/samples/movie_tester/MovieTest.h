//
//  MovieTest.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#ifndef __gl__MovieTest__
#define __gl__MovieTest__

#include "app/ViewerApp.h"

#include "gl/Texture.h"
#include "gl/ShaderLibrary.h"

#include "video/MovieController.h"
#include "video/CameraController.h"

namespace kinski
{
    namespace gl
    {
        class QuadWarp
        {
        public:
            
            QuadWarp():
            m_grid_num_w(10),
            m_grid_num_h(10)
            {
                m_control_points = {gl::vec2(0, 0), gl::vec2(1, 0), gl::vec2(0, 1), gl::vec2(1, 1)};
            }
            
            void init()
            {
                // adjust grid density here
                auto geom = gl::Geometry::createPlane(1, 1, m_grid_num_w, m_grid_num_h);
                for(auto &v : geom->vertices()){ v += vec3(0.5f, 0.5f, 0.f); }
                geom->computeBoundingBox();
                gl::Shader shader;
                shader.loadFromData(read_file("quad_warp.vert"), unlit_frag);
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
            
            void render_output(gl::Texture &the_texture)
            {
                if(!the_texture){ return; }
                if(!m_mesh){ init(); }
                
                m_mesh->material()->textures() = {the_texture};
                
                auto cp = m_control_points;
                for(auto &p : cp){ p.y = 1.f - p.y; }
                m_mesh->material()->uniform("u_control_points", cp);
                
                gl::ScopedMatrixPush model(MODEL_VIEW_MATRIX), projection(PROJECTION_MATRIX);
                gl::setMatrices(m_camera);
                gl::drawMesh(m_mesh);
            }
            
            void render_control_points()
            {
                if(!m_handle_material){ init(); }
                
//                const gl::Color colors[4] = {gl::COLOR_RED, gl::COLOR_GREEN, gl::COLOR_BLUE,
//                    gl::COLOR_YELLOW};
                
                for(int i = 0; i < m_control_points.size(); i++)
                {
//                    m_handle_material->setDiffuse(colors[i]);
                    gl::drawCircle(m_control_points[i] * gl::windowDimension(), 20.f, false,
                                   m_handle_material);
                }
            }
            
            void render_grid()
            {
                if(!m_grid_mesh){ init(); }
                auto cp = m_control_points;
                for(auto &p : cp){ p.y = 1.f - p.y; }
                m_grid_mesh->material()->uniform("u_control_points", cp);
                
                gl::ScopedMatrixPush model(MODEL_VIEW_MATRIX), projection(PROJECTION_MATRIX);
                gl::setMatrices(m_camera);
                gl::drawMesh(m_grid_mesh);
            }
            
            uint32_t grid_num_w() const { return m_grid_num_w; };
            uint32_t grid_num_h() const { return m_grid_num_h; };
            
            const gl::vec2& control_point(int the_x, int the_y) const
            {
                return control_point(the_x, the_y);
            }
            
            gl::vec2& control_point(int the_x, int the_y)
            {
                the_x = clamp(the_x, 0, 1);
                the_y = clamp(the_y, 0, 1);
                return m_control_points[the_x + 2 * the_y];
            }
            
            void set_control_point(int the_x, int the_y, const gl::vec2 &the_point)
            {
                control_point(the_x, the_y) = the_point;
            }
            
        private:
            
            uint32_t m_grid_num_w, m_grid_num_h;
            
            std::vector<gl::vec2> m_control_points;
            std::set<uint32_t> m_selected_indices;
            
            gl::OrthographicCamera::Ptr
            m_camera = gl::OrthographicCamera::create(0, 1, 0, 1, 0, 1);
            
            gl::MeshPtr m_mesh, m_grid_mesh;
            gl::MaterialPtr m_handle_material;
        };
    }
    
    class MovieTest : public ViewerApp
    {
    private:
        
        gl::QuadWarp m_quad_warp;
        
        video::MovieControllerPtr m_movie = video::MovieController::create();
        video::CameraControllerPtr m_camera_control = video::CameraController::create();
        vector<gl::Texture> m_textures{4};
        
        // properties
        Property_<string>::Ptr m_movie_path = Property_<string>::create("movie path", "");
        Property_<float>::Ptr m_movie_speed = Property_<float>::create("movie speed", 1.f);
        
    public:
        
        void setup();
        void update(float timeDelta);
        void draw();
        void got_message(const std::vector<uint8_t> &the_data);
        void fileDrop(const MouseEvent &e, const std::vector<std::string> &files);
        void tearDown();
        void update_property(const Property::ConstPtr &theProperty);
        
        void keyPress(const KeyEvent &e);
        
        void on_movie_load();
        
    };
}// namespace kinski

#endif /* defined(__gl__MovieTest__) */
