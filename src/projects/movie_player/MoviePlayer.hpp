//
//  MoviePlayer.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//
#pragma once

#include "app/ViewerApp.h"
#include "app/WarpComponent.h"

#include "gl/Texture.hpp"

#include "video/MovieController.h"
#include "video/CameraController.h"

namespace kinski
{
    class MoviePlayer : public ViewerApp
    {
    private:

        enum TextureEnum{TEXTURE_INPUT = 0, TEXTURE_OUTPUT = 1};

        WarpComponent::Ptr m_warp;

        video::MovieControllerPtr m_movie = video::MovieController::create();
        video::CameraControllerPtr m_camera_control = video::CameraController::create();
        bool m_reload_movie = false;
        
        // properties
        Property_<string>::Ptr m_movie_path = Property_<string>::create("movie path", "");
        Property_<float>::Ptr m_movie_speed = Property_<float>::create("movie speed", 1.f);
        Property_<bool>::Ptr m_use_warping = Property_<bool>::create("use warping", false);
        
        std::string secs_to_time_str(float the_secs) const;
        
    public:

        MoviePlayer(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
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
        
        bool save_settings(const std::string &path = "") override;
        bool load_settings(const std::string &path = "") override;

    };
}// namespace kinski
