//
//  MovieTest.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#ifndef __gl__MovieTest__
#define __gl__MovieTest__

#include "app/ViewerApp.h"

#include "gl/Texture.h"

#include "MovieController.h"
#include "CameraController.h"

namespace kinski
{
    class MovieTest : public ViewerApp
    {
    private:
        
        gl::Font m_font;
        MovieController m_movie;
        CameraController m_camera_control;
        vector<gl::Texture> m_textures{4};
        
        // properties
        Property_<string>::Ptr m_movie_path = Property_<string>::create("movie path", "");
        Property_<float>::Ptr m_movie_speed = Property_<float>::create("movie speed", 1.f);
        
    public:
        
        void setup();
        void update(float timeDelta);
        void draw();
        void got_message(const std::vector<uint8_t> &the_data);
        void fileDrop(const std::vector<std::string> &files);
        void tearDown();
        void updateProperty(const Property::ConstPtr &theProperty);
        
        void keyPress(const KeyEvent &e);
        
        void on_movie_load();
        
    };
}// namespace kinski

#endif /* defined(__gl__MovieTest__) */
