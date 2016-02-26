//
//  FractureApp.h
//  gl
//
//  Created by Fabian on 01/02/15.
//
//

#pragma once

#include "app/ViewerApp.h"
#include "app/LightComponent.h"

#include "gl/Fbo.hpp"

// module headers
#include "physics/physics_context.h"
#include "syphon/SyphonConnector.h"
#include "video/MovieController.h"

namespace kinski
{
    class FractureApp : public ViewerApp
    {
    private:
        
        enum ViewType{VIEW_NOTHING = 1, VIEW_DEBUG = 1, VIEW_OUTPUT = 2};
        enum TextureEnum{TEXTURE_OUTER = 0, TEXTURE_INNER = 1, TEXTURE_SYPHON = 2};
        
        gl::MeshPtr m_mesh;
        physics::physics_context m_physics;
        video::MovieControllerPtr m_movie, m_crosshair_movie;
        LightComponent::Ptr m_light_component;
        
        Property_<std::string>::Ptr
        m_model_path = Property_<std::string>::create("Model path", ""),
        m_crosshair_path = Property_<std::string>::create("Crosshair path", "crosshair.png");
        
        Property_<std::vector<std::string>>::Ptr
        m_texture_paths = Property_<std::vector<std::string>>::create("texture paths", {});
        
        Property_<bool>::Ptr
        m_physics_running = Property_<bool>::create("physics running", true),
        m_physics_debug_draw = Property_<bool>::create("physics debug draw", true);
        
        Property_<uint32_t>::Ptr
        m_num_fracture_shards = Property_<uint32_t>::create("num fracture shards", 20);
        
        Property_<float>::Ptr
        m_gravity = Property_<float>::create("gravity", 9.81f),
        m_friction = Property_<float>::create("friction", .6f),
        m_breaking_thresh = Property_<float>::create("breaking threshold", 2.4f),
        m_shoot_velocity = Property_<float>::create("shoot velocity", 40.f),
        m_shots_per_sec = Property_<float>::create("shots per second", 3.f);
        
        Property_<glm::vec3>::Ptr
        m_obj_scale = Property_<glm::vec3>::create("object scale", glm::vec3(.5f));
        
        physics::btCollisionShapePtr m_box_shape;
        gl::GeometryPtr m_box_geom;
        
        bool m_needs_refracture = true;
        std::list<physics::VoronoiShard> m_voronoi_shards;
        
        // gui stuff
        gl::CameraPtr m_gui_cam;
        gl::Texture m_crosshair_tex;
        std::vector<glm::vec2> m_crosshair_pos;
        float m_time_since_last_shot = 0;
        
        // fbo / syphon stuff
        std::vector<gl::Fbo> m_fbos{5};
        gl::CameraPtr m_fbo_cam;
        Property_<glm::vec3>::Ptr
        m_fbo_cam_pos = Property_<glm::vec3>::create("fbo camera position", glm::vec3(0, 0, 5.f));
        
        Property_<glm::vec2>::Ptr
        m_fbo_resolution = Property_<glm::vec2>::create("Fbo resolution", glm::vec2(1280, 640));
        
        Property_<uint32_t>::Ptr
        m_view_type = RangedProperty<uint32_t>::create("view type", VIEW_OUTPUT, 0, 2);
        
        // output via Syphon
        syphon::Output m_syphon;
        Property_<bool>::Ptr m_use_syphon = Property_<bool>::create("Use syphon", false);
        Property_<std::string>::Ptr m_syphon_server_name =
        Property_<std::string>::create("Syphon server name", "fracture");
        
        // post processing (Dof)
        gl::MaterialPtr m_dof_material;
        
        void shoot_box(const gl::Ray &the_ray, float the_velocity,
                       const glm::vec3 &the_half_extents = glm::vec3(.5f));
        
        void fracture_test(uint32_t num_shards);
        
        void reset_scene();
        
    public:
        
        void setup() override;
        void update(float timeDelta) override;
        void draw() override;
        void resize(int w ,int h) override;
        void keyPress(const KeyEvent &e) override;
        void keyRelease(const KeyEvent &e) override;
        void mousePress(const MouseEvent &e) override;
        void mouseRelease(const MouseEvent &e) override;
        void mouseMove(const MouseEvent &e) override;
        void mouseDrag(const MouseEvent &e) override;
        void mouseWheel(const MouseEvent &e) override;
        void got_message(const std::vector<uint8_t> &the_message) override;
        void fileDrop(const MouseEvent &e, const std::vector<std::string> &files) override;
        void tearDown() override;
        void update_property(const Property::ConstPtr &theProperty) override;
    };
}// namespace kinski