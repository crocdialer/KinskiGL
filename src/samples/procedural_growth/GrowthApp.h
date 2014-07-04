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
        
        
        // movie
        MovieController m_movie;
        
        // light controls
        LightComponent::Ptr m_light_component;
        
        gl::MeshPtr m_mesh, m_bounding_mesh;
        
        // our Lindemayer-System object
        LSystem m_lsystem;
        
        //! holds an shader programm, containing a geomtry shader for drawing the lines
        // created by a lsystem as more complex geometry
        //
        gl::Shader m_lsystem_shader;
        
        //! needs to recalculate
        bool m_dirty_lsystem = false;
        
        //! animate fractal growth
        animation::AnimationPtr m_growth_animation;
        
        // Properties
        RangedProperty<uint32_t>::Ptr m_max_index = RangedProperty<uint32_t>::create("max index",
                                                                                     0, 0, 2000000);
        
        Property_<uint32_t>::Ptr m_num_iterations = Property_<uint32_t>::create("num iterations", 2);
        Property_<glm::vec3>::Ptr m_branch_angles = Property_<glm::vec3>::create("branch angles",
                                                                                     glm::vec3(90));
        Property_<glm::vec3>::Ptr m_branch_randomness = Property_<glm::vec3>::create("branch randomness",
                                                                                 glm::vec3(0));
        RangedProperty<float>::Ptr m_increment = RangedProperty<float>::create("growth increment",
                                                                                1.f, 0.f, 1000.f);
        RangedProperty<float>::Ptr m_increment_randomness = RangedProperty<float>::create("growth increment randomness",
                                                                               0.f, 0.f, 1000.f);
        RangedProperty<float>::Ptr m_diameter = RangedProperty<float>::create("diameter",
                                                                              1.f, 0.f, 10.f);
        RangedProperty<float>::Ptr m_diameter_shrink = RangedProperty<float>::create("diameter shrink factor",
                                                                                     1.f, 0.f, 5.f);
        
        RangedProperty<float>::Ptr m_cap_bias = RangedProperty<float>::create("cap bias",
                                                                              0.f, 0.f, 10.f);
        
        Property_<std::string>::Ptr m_axiom = Property_<std::string>::create("Axiom", "f");
        std::vector<Property_<std::string>::Ptr> m_rules =
        {
            Property_<std::string>::create("Rule 1", "f = f - h"),
            Property_<std::string>::create("Rule 2", "h = f + h"),
            Property_<std::string>::create("Rule 3", ""),
            Property_<std::string>::create("Rule 4", "")
        };
        
        Property_<bool>::Ptr m_animate_growth = Property_<bool>::create("animate growth", false);
        RangedProperty<float>::Ptr m_animation_time = RangedProperty<float>::create("animation time",
                                                                               5.f, 0.f, 120.f);
        
        void refresh_lsystem();
        
        void parse_line(const std::string &line);
        
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
