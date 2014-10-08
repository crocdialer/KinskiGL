//
//  EmptySample.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#ifndef __gl__KristallApp__
#define __gl__KristallApp__

#include "app/ViewerApp.h"
#include "gl/Texture.h"
#include "gl/Fbo.h"
#include "core/Animation.h"
#include "core/Timer.h"
#include "LSystem.h"
#include "ParticleSystem.hpp"

// control components
#include "app/LightComponent.h"
#include "app/Object3DComponent.h"
#include "app/MaterialComponent.h"

// module headers
#include "SyphonConnector.h"
#include "Sound.h"
#include "MovieController.h"
#include "DMXController.h"

// networking
#include "core/networking.h"

// measuring, filters
#include "core/Measurement.hpp"

namespace kinski
{
    // audio enum and type defs
    enum FrequencyBand{FREQ_LOW = 0, FREQ_MID_LOW = 1, FREQ_MID_HIGH = 2, FREQ_HIGH = 3};
    typedef std::map<FrequencyBand, std::pair<float, float>> FrequencyRangeMap;
    typedef std::map<FrequencyBand, float> FrequencyVolumeMap;
    
    class KristallApp : public ViewerApp
    {
        
    private:
        
        net::tcp_server m_tcp_server;
        std::vector<net::tcp_connection_ptr> m_tcp_connections;
        
        // game logic
        enum GameState{READY_PHASE, IMPACT_PHASE, GROW_PHASE, DECAY_PHASE, SCORE_PHASE,
            IDLE_PHASE, SCORE_MINIMUM, SCORE_MAXIMUM};
        GameState m_gamestate = READY_PHASE;
        
        enum AnimationEnum{LIGHT_0 = 1, LIGHT_1 = 2,  GROWTH = 4, DECAY = 5, MESH_ROTATION = 6,
            MESH_FADE = 7};
        
        enum TextureEnum{MASK_TEX = 0, MOVIE_TEX = 2, OUTPUT_TEX = 3, DIFFUSE_TEX_0 = 4,
            DIFFUSE_TEX_1 = 5, OFFSCREEN_TEX_0 = 6, OFFSCREEN_TEX_1 = 7, DIFFUSE_TEX_2 = 8,
            EMPTY_TEX = 9};
        
        // experimental voice values
        FrequencyRangeMap m_frequency_map =
        {
            {FREQ_LOW, std::pair<float, float>(20.f, 200.f)},
            {FREQ_MID_LOW, std::pair<float, float>(200.f, 400.f)},
            {FREQ_MID_HIGH, std::pair<float, float>(400.f, 1200.f)},
            {FREQ_HIGH, std::pair<float, float>(1200.f, 22050.f)}
        };
        
        struct CompositingAssets
        {
            gl::Texture template_img, mask_img, syphon_img, empty_tex, background_tex;
            bool use_background = false;
        } m_comp_assets;
        
        // last measured values
        std::vector<float> m_last_volumes;
        
        // overall volume
        float m_last_value;
        bool m_burst_lvl = false;
        
        kinski::Stopwatch m_stop_watch;
        
        kinski::Timer m_idle_timer, m_ready_timer, m_decay_timer, m_test_timer;
        
        enum ViewOption{VIEW_DEBUG = 0, VIEW_OUTPUT = 1, VIEW_OUTPUT_DEBUG = 2, VIEW_TEMPLATE = 3,
            VIEW_NOTHING = 4};
        
        gl::Font m_font;
        std::vector<gl::Texture> m_textures{10};
        
        // movies
        std::vector<MovieController> m_movies{2};
        
        // audio samples
        
        // recording assets
        audio::SoundPtr m_sound_recording;
        std::vector<float> m_sound_spectrum;
        std::vector<kinski::Measurement<float>> m_sound_values{256};
        
        // light controls
        LightComponent::Ptr m_light_component;
        gl::Object3DPtr m_light_root{new gl::Object3D};
        Property_<float>::Ptr
        m_light_elevation = Property_<float>::create("light elevation speed", 5.f);
        
        // materials
        MaterialComponent::Ptr m_material_component;
        std::vector<gl::Color> m_color_table;
        
        typedef std::map<FrequencyBand, gl::MeshPtr> MeshMap;
        MeshMap m_mesh_map;
        
        gl::MeshPtr m_mesh, m_bounding_mesh;
        
        // occluder mechanism
        gl::GeometryPtr m_occluder_geom;
        gl::MaterialPtr m_occluder_mat;
        std::vector<gl::MeshPtr> m_occluding_meshes{20};
        Object3DComponent::Ptr m_occluder_component;
        
        std::vector<LSystem> m_lsystems;
        LSystem m_lsystem;
        
        // the particle system used to blast the mesh in pieces
        gl::ParticleSystem m_particle_system;
        
        //! holds some shader programs, containing geomtry shaders for drawing the lines
        // created by a lsystem as more complex geometry
        //
        std::vector<gl::Shader> m_lsystem_shaders{10};
        
        //! needs to recalculate
        bool m_dirty_lsystem = false;
        
        //! animate fractal growth
        animation::AnimationPtr m_growth_animation;
        std::vector<gl::Mesh::Entry> m_entries;
        
        //! animations
        std::vector<animation::AnimationPtr> m_animations{10};
        
        /////////////////////////////////////////////////////////////////
        
        // output via Syphon
        gl::SyphonConnector m_syphon;
        Property_<bool>::Ptr m_use_syphon = Property_<bool>::create("Use syphon", false);
        Property_<std::string>::Ptr m_syphon_server_name =
            Property_<std::string>::create("Syphon server name", "schwarzer kristall");
        
        /////////////////////////////////////////////////////////////////
        
        // offscreen rendering
        gl::Scene m_debug_scene;
        
        std::vector<gl::Fbo> m_offscreen_fbos{2};
        
        Property_<glm::vec2>::Ptr m_fbo_size = Property_<glm::vec2>::create("FBO size", glm::vec2(1920, 460));
        
        RangedProperty<float>::Ptr m_fbo_cam_distance =
            RangedProperty<float>::create("FBO cam distance", 4.f, 0, 4000);
        
        RangedProperty<float>::Ptr m_fbo_cam_height =
            RangedProperty<float>::create("FBO cam height", 0.f, -1000, 1000);
        
        RangedProperty<float>::Ptr m_fbo_fov = RangedProperty<float>::create("FBO fov", 45.f, 0, 180);
        
        gl::PerspectiveCamera::Ptr m_offscreen_camera;
        
        gl::MeshPtr m_camera_mesh;
        
        gl::MaterialPtr m_compositing_mat;
        
        /////////////////////////////////////////////////////////////////
        
        // Properties
        
        // view option
        RangedProperty<uint32_t>::Ptr m_view_option =
        RangedProperty<uint32_t>::create("View option",
                                         VIEW_DEBUG, 0, 4);
        
        // template background image
        Property_<std::string>::Ptr
        m_template_image = Property_<std::string>::create("template image", "template.png");
        
        Property_<uint32_t>::Ptr m_num_iterations = Property_<uint32_t>::create("num iterations", 2);
        Property_<glm::vec3>::Ptr m_branch_angles = Property_<glm::vec3>::create("branch angles",
                                                                                     glm::vec3(90));
        Property_<glm::vec3>::Ptr m_branch_randomness =
            Property_<glm::vec3>::create("branch randomness",
                                         glm::vec3(0));
        RangedProperty<float>::Ptr m_increment = RangedProperty<float>::create("growth increment",
                                                                                1.f, 0.f, 1000.f);
        RangedProperty<float>::Ptr m_increment_randomness =
            RangedProperty<float>::create("growth increment randomness",
                                          0.f, 0.f, 1000.f);
        RangedProperty<float>::Ptr m_diameter = RangedProperty<float>::create("diameter",
                                                                              1.f, 0.f, 100.f);
        RangedProperty<float>::Ptr m_diameter_shrink =
            RangedProperty<float>::create("diameter shrink factor",
                                          1.f, 0.f, 5.f);
        
        RangedProperty<float>::Ptr m_cap_bias = RangedProperty<float>::create("cap bias",
                                                                              0.f, 0.f, 100.f);
        
        RangedProperty<float>::Ptr m_split_limit =
            RangedProperty<float>::create("geometry split limit",
                                          60.f, 0.f, 200.f);
        
        Property_<std::string>::Ptr m_axiom = Property_<std::string>::create("Axiom", "f");
        std::vector<Property_<std::string>::Ptr> m_rules =
        {
            Property_<std::string>::create("Rule 1", "f = f - h"),
            Property_<std::string>::create("Rule 2", "h = f + h"),
            Property_<std::string>::create("Rule 3", ""),
            Property_<std::string>::create("Rule 4", "")
        };
        
        Property_<glm::vec3>::Ptr
        m_aabb_min = Property_<glm::vec3>::create("aabb min", glm::vec3(-59, 0, -59)),
        m_aabb_max = Property_<glm::vec3>::create("aabb max", glm::vec3(59, 1129, 59));
        
        Property_<bool>::Ptr m_use_bounding_mesh = Property_<bool>::create("use bounding mesh", false);
        Property_<bool>::Ptr m_animate_growth = Property_<bool>::create("animate growth", false);
        
        Property_<bool>::Ptr m_use_particle_simulation =
            Property_<bool>::create("Enable particle simulation", false);
        RangedProperty<float>::Ptr m_particle_bounce =
        RangedProperty<float>::create("particle bounce", .4f, 0, 1);
        
        RangedProperty<float>::Ptr m_particle_gravity =
            RangedProperty<float>::create("particle gravity",
                                          150.f, 0.f, 500.f);
        
        // timing values
        RangedProperty<float>::Ptr
        m_time_growth = RangedProperty<float>::create("growth time", 5.f, 0.f, 60.f);
        
        // force multiplier
        RangedProperty<float>::Ptr
        m_force_multiplier = RangedProperty<float>::create("force mutliplier", 1.f, 0.f, 10.f),
        m_activity_thresh = RangedProperty<float>::create("activity threshold", .2f, 0.f, 1.f),
        m_burst_thresh = RangedProperty<float>::create("burst threshold", 1.f, 0.f, 10.f);
        
        RangedProperty<float>::Ptr
        m_smooth_inc = RangedProperty<float>::create("increase speed", .2f, 0, 1),
        m_smooth_dec = RangedProperty<float>::create("decrease speed", .04f, 0, 1);
        
        // timing values
        RangedProperty<float>::Ptr
        m_time_idle = RangedProperty<float>::create("idle time", 30.f, 0.f, 600.f),
        m_time_decay = RangedProperty<float>::create("decay time", 30.f, 0.f, 600.f),
        m_time_burst = RangedProperty<float>::create("burst time", .5f, 0.f, 60.f);
        
        // frequency values
        RangedProperty<float>::Ptr
        m_freq_low = RangedProperty<float>::create("frequency low", 200, 0.f, 22050.f),
        m_freq_mid_low = RangedProperty<float>::create("frequency mid low", 400, 0.f, 22050.f),
        m_freq_mid_high = RangedProperty<float>::create("frequency mid high", 1200, 0.f, 22050.f),
        m_freq_high = RangedProperty<float>::create("frequency high", 22050, 0.f, 22050.f);
        
        // dmx vals
        DMXController m_dmx_control;
        RangedProperty<int>::Ptr
        m_dmx_start_index = RangedProperty<int>::create("DMX start index", 1, 0, 255);
        Property_<gl::Color>::Ptr m_dmx_color = Property_<gl::Color>::create("DMX color", gl::COLOR_OLIVE);
        
        void refresh_lsystem(LSystem &the_system, int num_iterations, float sz = 1.f);
        
        void update_animations(float time_delta);
        
        void start_decay(float fade_time);
        
        gl::Texture composite_output_image();
        
        void change_gamestate(GameState the_state);

        void set_light_animation(float force);
        void fade_in_background();
        
        float get_volume_for_subspectrum(float from_freq, float to_freq);
        float get_volume_for_freq_band(FrequencyBand bnd);
        
        void setup_lsystem(int the_index);
        
    public:
        
        void setup();
        void update(float timeDelta);
        void draw();
        void resize(int w ,int h);
        void keyPress(const KeyEvent &e);
        void keyRelease(const KeyEvent &e);
        void mousePress(const MouseEvent &e);
        void mouseRelease(const MouseEvent &e);
        void mouseMove(const MouseEvent &e);
        void mouseDrag(const MouseEvent &e);
        void mouseWheel(const MouseEvent &e);
        void fileDrop(const std::vector<std::string> &files);
        void got_message(const std::vector<uint8_t> &the_message);
        void tearDown();
        void updateProperty(const Property::ConstPtr &theProperty);
        
        virtual void save_settings(const std::string &path = "");
        virtual void load_settings(const std::string &path = "");
    };
}// namespace kinski

#endif /* defined(__gl__EmptySample__) */
