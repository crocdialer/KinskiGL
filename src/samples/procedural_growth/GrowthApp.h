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
#include "kinskiGL/Texture.h"
#include "LSystem.h"

namespace kinski
{
    class GrowthApp : public ViewerApp
    {
    private:
        
        gl::Font m_font;
        std::vector<gl::Texture> m_textures{4};
        
        gl::MeshPtr m_mesh;
        LSystem m_lsystem;
        
        //! needs to recalculate
        bool m_dirty_lsystem = false;
        
        // Properties
        RangedProperty<uint32_t>::Ptr m_max_index = RangedProperty<uint32_t>::create("max index",
                                                                                     0, 0, 2000000);
        
        Property_<uint32_t>::Ptr m_num_iterations = Property_<uint32_t>::create("num iterations", 2);
        Property_<glm::vec3>::Ptr m_branch_angles = Property_<glm::vec3>::create("branch angles",
                                                                                     glm::vec3(90));
        RangedProperty<float>::Ptr m_increment = RangedProperty<float>::create("growth increment",
                                                                                1.f, 0.f, 1000.f);
        
        Property_<std::string>::Ptr m_axiom = Property_<std::string>::create("Axiom", "f");
        std::vector<Property_<std::string>::Ptr> m_rules =
        {
            Property_<std::string>::create("Rule 1", "f = f - h"),
            Property_<std::string>::create("Rule 2", "h = f + h"),
            Property_<std::string>::create("Rule 3", ""),
            Property_<std::string>::create("Rule 4", "")
        };
        
        void refresh_lsystem();
        
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
