//
//  BlockbusterApp.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#ifndef __gl__BlockbusterApp__
#define __gl__BlockbusterApp__

#include "app/ViewerApp.h"
#include "app/RemoteControl.h"
#include "app/LightComponent.h"

// module
#include "opencl/ParticleSystem.hpp"
#include "video/CameraController.h"
#include "video/MovieController.h"
#include "openni/OpenNIConnector.h"
#include "syphon/SyphonConnector.h"

namespace kinski
{
    class BlockbusterApp : public ViewerApp
    {
    private:
        
        enum ViewType{VIEW_NOTHING = 0, VIEW_DEBUG = 1, VIEW_OUTPUT = 2};
        enum TextureEnum{TEXTURE_DEPTH = 0, TEXTURE_MOVIE = 1, TEXTURE_SYPHON = 2};
        
        // web interface remote control
        RemoteControl m_remote_control;
        
        gl::OpenNIConnector::Ptr m_open_ni;
        
        video::MovieControllerPtr m_movie;
        
        LightComponent::Ptr m_light_component;
        
        gl::MeshPtr m_mesh;
        std::vector<glm::vec3> m_user_positions;
        gl::ParticleSystem m_psystem;
        
        gl::Texture m_texture_input;
        
        gl::Shader m_block_shader, m_block_shader_shadows;
        
        Property_<std::string>::Ptr
        m_media_path = Property_<std::string>::create("media path", "");
        
        bool m_dirty = true;
        bool m_has_new_texture = false;
        
        // fbo / syphon stuff
        std::vector<gl::Fbo> m_fbos{2};
        gl::CameraPtr m_fbo_cam;
        Property_<glm::vec3>::Ptr
        m_fbo_cam_pos = Property_<glm::vec3>::create("fbo camera position", glm::vec3(0, 0, 5.f));
        
        Property_<float>::Ptr
        m_fbo_cam_fov = Property_<float>::create("fbo camera fov", 45.f);
        
        Property_<glm::vec2>::Ptr
        m_fbo_resolution = Property_<glm::vec2>::create("Fbo resolution", glm::vec2(1280, 640));
        
        Property_<uint32_t>::Ptr
        m_view_type = RangedProperty<uint32_t>::create("view type", VIEW_OUTPUT, 0, 2);
        
        // output via Syphon
        syphon::Output m_syphon;
        Property_<bool>::Ptr m_use_syphon = Property_<bool>::create("Use syphon", false);
        Property_<std::string>::Ptr m_syphon_server_name =
        Property_<std::string>::create("Syphon server name", "blockbuster");
        
        Property_<uint32_t>::Ptr
        m_num_tiles_x = Property_<uint32_t>::create("num tiles x", 16),
        m_num_tiles_y = Property_<uint32_t>::create("num tiles y", 10),
        m_spacing_x = Property_<uint32_t>::create("spacing x", 10),
        m_spacing_y = Property_<uint32_t>::create("spacing y", 10),
        m_border = Property_<uint32_t>::create("border", 1);
        
        Property_<float>::Ptr
        m_block_length = Property_<float>::create("block length", 1.f),
        m_block_width = Property_<float>::create("block width", 1.f),
        m_block_width_multiplier = Property_<float>::create("block width multiplier", 1.f),
        m_depth_min = Property_<float>::create("depth min", 1.f),
        m_depth_max = Property_<float>::create("depth max", 3.f),
        m_depth_multiplier = Property_<float>::create("depth mutliplier", 10.f),
        m_depth_smooth_fall = Property_<float>::create("depth smooth falling", .95f),
        m_depth_smooth_rise = Property_<float>::create("depth smooth rising", .7f),
        m_poisson_radius = Property_<float>::create("poisson radius", 3.f);
        
        Property_<bool>::Ptr
        m_mirror_img = Property_<bool>::create("mirror image", false),
        m_use_shadows = Property_<bool>::create("use shadows", true);
        
        void init_shaders();
        gl::MeshPtr create_mesh();
        glm::vec3 click_pos_on_ground(const glm::vec2 click_pos);
        
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
        void updateProperty(const Property::ConstPtr &theProperty) override;
        
        bool save_settings(const std::string &path = "") override;
        bool load_settings(const std::string &path = "") override;
    };
}// namespace kinski

#endif /* defined(__gl__BlockbusterApp__) */
