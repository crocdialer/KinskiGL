//
//  CoreVideoTest.h
//  kinskiGL
//
//  Created by Fabian on 29/01/14.
//
//

#ifndef __kinskiGL__CoreVideoTest__
#define __kinskiGL__CoreVideoTest__

#include "kinskiApp/ViewerApp.h"

#include "kinskiGL/Texture.h"
#include "MovieController.h"

namespace kinski
{
    class CoreVideoTest : public ViewerApp
    {
    private:
        
        gl::Font m_font;
        MovieController m_movie;
        vector<gl::Texture> m_textures{4};
        
        // properties
        Property_<string>::Ptr m_movie_path = Property_<string>::create("movie path", "");
        Property_<float>::Ptr m_movie_speed = Property_<float>::create("movie speed", 1.f);
        
    public:
        
        void setup();
        void update(float timeDelta);
        void draw();
        void got_message(const std::string &the_message);
        void tearDown();
        void updateProperty(const Property::ConstPtr &theProperty);
        
        void keyPress(const KeyEvent &e);
        
    };
}// namespace kinski

#endif /* defined(__kinskiGL__CoreVideoTest__) */
