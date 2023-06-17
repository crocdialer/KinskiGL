// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  TouchSample.hpp
//
//  Created by Fabian on 04/03/16.

#pragma once

#include "app/ViewerApp.hpp"
#include "gl/Noise.hpp"

namespace kinski
{
    class TouchSample : public ViewerApp
    {
    private:
        
        enum FontEnum{FONT_NORMAL = 0, FONT_LARGE = 1};
        enum TextureEnum{TEXTURE_SIMPLEX = 0};
        
        std::set<const Touch*> m_current_touches;
        
        gl::Noise m_noise;
        
        gl::FboPtr m_offscreen_fbo;
        
        // properties
        crocore::Property_<float>::Ptr m_circle_radius = crocore::Property_<float>::create("circle radius", 65.f);
        
    public:
        TouchSample(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
        void setup() override;
        void update(float timeDelta) override;
        void draw() override;
        void resize(int w ,int h) override;
        void key_press(const KeyEvent &e) override;
        void key_release(const KeyEvent &e) override;
        void mouse_press(const MouseEvent &e) override;
        void mouse_release(const MouseEvent &e) override;
        void mouse_move(const MouseEvent &e) override;
        void mouse_drag(const MouseEvent &e) override;
        void mouse_wheel(const MouseEvent &e) override;
        void touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void file_drop(const MouseEvent &e, const std::vector<std::string> &files) override;
        void teardown() override;
        void update_property(const crocore::PropertyConstPtr &theProperty) override;
    };
}// namespace kinski

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::TouchSample>(argc, argv);
    spdlog::info("local ip: {}", crocore::net::local_ip());
    return theApp->run();
}
