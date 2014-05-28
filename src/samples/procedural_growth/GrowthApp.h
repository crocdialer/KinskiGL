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
        
        // Properties
        
        Property_<uint32_t>::Ptr m_num_iterations = Property_<uint32_t>::create("num iterations", 2);
        
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
