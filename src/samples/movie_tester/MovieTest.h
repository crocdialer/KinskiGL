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
            
            QuadWarp()
            {
                
            }
            
            void init()
            {
                // adjust grid density here
                auto geom = gl::Geometry::createPlane(1, 1, 4, 4);
                for(auto &v : geom->vertices()){ v += vec3(0.5f, 0.5f, 0.f); }
                geom->computeBoundingBox();
                gl::Shader shader;
                shader.loadFromData(read_file("quad_warp.vert"), unlit_frag);
                auto mat = gl::Material::create(shader);
                mat->setDepthTest(false);
                mat->setDepthWrite(false);
                m_mesh = gl::Mesh::create(geom, mat);
                
                m_handle_material = gl::Material::create();
                
                m_control_points = {gl::vec2(0, 0), gl::vec2(0, 1), gl::vec2(1, 1), gl::vec2(1, 0)};
            }
            
            void render_output(gl::Texture &the_texture)
            {
                if(!the_texture){ return; }
                if(!m_mesh){ init(); }
                
                m_mesh->material()->textures() = {the_texture};
                m_mesh->material()->uniform("u_control_points", m_control_points);
                
                gl::ScopedMatrixPush model(MODEL_VIEW_MATRIX), projection(PROJECTION_MATRIX);
                gl::setMatrices(m_camera);
                gl::drawMesh(m_mesh);
            }
            
            void render_control_points()
            {
                const gl::Color colors[4] = {gl::COLOR_RED, gl::COLOR_GREEN, gl::COLOR_BLUE,
                    gl::COLOR_YELLOW};
                
                for(int i = 0; i < m_control_points.size(); i++)
                {
                    m_handle_material->setDiffuse(colors[i]);
                    gl::drawCircle(m_control_points[i] * gl::windowDimension(), 20.f, false,
                                   m_handle_material);
                }
            }
            
//            uint32_t num_sub_divisions_w() const { return m_num_w; };
//            uint32_t num_sub_divisions_h() const { return m_num_h; };
            
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
            
            std::vector<gl::vec2> m_control_points;
            std::set<uint32_t> m_selected_indices;
            
            gl::OrthographicCamera::Ptr
            m_camera = gl::OrthographicCamera::create(0, 1, 0, 1, 0, 1);
            
            gl::MeshPtr m_mesh;
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
