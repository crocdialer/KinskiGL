//
//  MovieTimeshift.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#ifndef __gl__MovieTimeshift__
#define __gl__MovieTimeshift__

#include "app/ViewerApp.h"
#include "core/Timer.h"

#include "MovieController.h"
#include "CameraController.h"

namespace kinski
{
    class MovieTimeshift : public ViewerApp
    {
    private:
        
        enum TextureEnum { TEXTURE_DIFFUSE = 0, TEXTURE_NOISE = 1};
        
        MovieController m_movie;
        CameraController m_camera;
        
        bool m_needs_movie_refresh = false;
        
        std::vector<uint8_t> m_camera_data;
        
        gl::ArrayTexture m_array_tex;
        gl::MaterialPtr m_custom_mat;
        
        Timer m_timer_update_noise;
        
        
        // properties
        Property_<bool>::Ptr m_use_camera = Property_<bool>::create("use camera", false);
        Property_<string>::Ptr m_movie_path = Property_<string>::create("movie path", "");
        Property_<float>::Ptr m_movie_speed = Property_<float>::create("movie speed", 1.f);
        
        gl::Texture create_noise_tex(float seed = 0.025f);
        
    public:
        
        void setup();
        void update(float timeDelta);
        void draw();
        void got_message(const std::vector<uint8_t> &the_data);
        void fileDrop(const MouseEvent &e, const std::vector<std::string> &files);
        void tearDown();
        void updateProperty(const Property::ConstPtr &theProperty);
        
        void keyPress(const KeyEvent &e);
        
        void on_movie_load();
        
    };
}// namespace kinski

#endif /* defined(__gl__MovieTimeshift__) */
