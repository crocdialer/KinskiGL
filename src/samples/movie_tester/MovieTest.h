//
//  MovieTest.h
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
    class MovieTest : public ViewerApp
    {
    private:
        
        WarpComponent::Ptr m_warp;
        
        video::MovieControllerPtr m_movie = video::MovieController::create();
        video::CameraControllerPtr m_camera_control = video::CameraController::create();
        vector<gl::Texture> m_textures{4};
        
        // properties
        Property_<string>::Ptr m_movie_path = Property_<string>::create("movie path", "");
        Property_<float>::Ptr m_movie_speed = Property_<float>::create("movie speed", 1.f);
        
    public:
        
        void setup();
        void update(float timeDelta);
        void draw();
        void got_message(const std::vector<uint8_t> &the_data);
        void fileDrop(const MouseEvent &e, const std::vector<std::string> &files);
        void tearDown();
        void update_property(const Property::ConstPtr &theProperty);
        
        void keyPress(const KeyEvent &e);
        
        void on_movie_load();
        
    };
}// namespace kinski