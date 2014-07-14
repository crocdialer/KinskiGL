//
//  EmptySample.h
//  kinskiGL
//
//  Created by Fabian on 29/01/14.
//
//

#ifndef __kinskiGL__GrowthApp__
#define __kinskiGL__GrowthApp__

#include "kinskiApp/ViewerApp.h"
#include "kinskiApp/LightComponent.h"
#include "kinskiGL/Texture.h"
#include "kinskiCore/Animation.h"
#include "LSystem.h"

// Movie playback with avfoundation
#include "MovieController.h"

// serial communication
#include "kinskiCore/Serial.h"

// audio
#include "RtAudio.h"
#include "Sound.h"

// networking
#include "kinskiCore/networking.h"

namespace kinski
{
    class GrowthApp : public ViewerApp
    {
    private:
        
        gl::Font m_font;
        std::vector<gl::Texture> m_textures{4};
        
        // Serial communication with Arduino device
        Property_<string>::Ptr m_arduino_device_name =
            Property_<string>::create("Arduino device name",
                                      "/dev/tty.usbmodem1411");
        // the serial device
        Serial m_serial;
        
        // sensor inputs
        string m_input_prefix = "analog_";
        std::vector<Measurement<float>> m_analog_in { Measurement<float>("Hammer Input") };
        
        // udp receiver
        net::udp_server m_udp_server;
        Property_<uint32_t>::Ptr m_local_udp_port = Property_<uint32_t>::create("udp port", 11111);
        
        // audio device
        RtAudio m_audio;
        
        // audio samples
        std::vector<audio::SoundPtr> m_samples;
        
        // movie
        MovieController m_movie;
        
        // light controls
        LightComponent::Ptr m_light_component;
        gl::Object3DPtr m_light_root{new gl::Object3D};
        Property_<float>::Ptr
        m_light_rotation = Property_<float>::create("light rotation speed", 5.f),
        m_light_elevation = Property_<float>::create("light elevation speed", 5.f);
        
//        float m_light_rotation, m_light_elevation;
        
        gl::MeshPtr m_mesh, m_bounding_mesh;
        
        // our Lindemayer-System object
        LSystem m_lsystem;
        
        //! holds some shader programs, containing geomtry shaders for drawing the lines
        // created by a lsystem as more complex geometry
        //
        std::vector<gl::Shader> m_lsystem_shaders{4};
        
        //! needs to recalculate
        bool m_dirty_lsystem = false;
        
        //! animate fractal growth
        animation::AnimationPtr m_growth_animation;
        gl::Mesh::Entry m_entry;
        
        //! animations
        std::vector<animation::AnimationPtr> m_animations{10};
        
        // Properties
        RangedProperty<uint32_t>::Ptr m_max_index = RangedProperty<uint32_t>::create("max index",
                                                                                     0, 0, 2000000);
        
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
        
        Property_<std::string>::Ptr m_axiom = Property_<std::string>::create("Axiom", "f");
        std::vector<Property_<std::string>::Ptr> m_rules =
        {
            Property_<std::string>::create("Rule 1", "f = f - h"),
            Property_<std::string>::create("Rule 2", "h = f + h"),
            Property_<std::string>::create("Rule 3", ""),
            Property_<std::string>::create("Rule 4", "")
        };
        
        Property_<bool>::Ptr m_use_bounding_mesh = Property_<bool>::create("use bounding mesh", false);
        Property_<bool>::Ptr m_animate_growth = Property_<bool>::create("animate growth", false);
        RangedProperty<float>::Ptr m_animation_time = RangedProperty<float>::create("animation time",
                                                                               5.f, 0.f, 120.f);
        
        Property_<uint32_t>::Ptr m_shader_index = Property_<uint32_t>::create("shader index", 0);
        
        void refresh_lsystem();
        void parse_line(const std::string &line);
        void animate_lights(float time_delta);
        void update_animations(float time_delta);
        
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
        void got_message(const std::vector<uint8_t> &the_message);
        void tearDown();
        void updateProperty(const Property::ConstPtr &theProperty);
    };
}// namespace kinski

#endif /* defined(__kinskiGL__EmptySample__) */
