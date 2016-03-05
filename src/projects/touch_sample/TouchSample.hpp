//
//  TouchSample.h
//  gl
//
//  Created by Fabian on 04/03/16.
//
//

#pragma once

#include "app/ViewerApp.h"
#include "gl/Noise.hpp"

namespace kinski
{
    class TouchSample : public ViewerApp
    {
    private:
        
        enum FontEnum{FONT_NORMAL = 0, FONT_LARGE = 1};
        
        std::set<const Touch*> m_current_touches;
        
        gl::Noise m_noise;
        
        // properties
        Property_<float>::Ptr m_circle_radius = Property_<float>::create("circle radius", 65.f);
        
    public:
        TouchSample(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
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
        void touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void fileDrop(const MouseEvent &e, const std::vector<std::string> &files) override;
        void tearDown() override;
        void update_property(const Property::ConstPtr &theProperty) override;
    };
}// namespace kinski