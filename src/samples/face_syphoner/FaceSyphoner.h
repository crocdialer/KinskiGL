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

#include "cv/CVThread.h"
#include "SyphonConnector.h"

namespace kinski
{
    class FaceSyphoner : public ViewerApp
    {
    private:
        
        gl::Font m_font;
        vector<gl::Texture> m_textures{4};
        
        // output via Syphon
        vector<gl::SyphonConnector> m_syphon{4};
        
        CVThreadPtr m_opencv = CVThread::create();
        
        Property_<bool>::Ptr m_use_syphon = Property_<bool>::create("Use syphon", false);
        Property_<std::string>::Ptr m_syphon_server_name =
        Property_<std::string>::create("Syphon server name", "facesyphoner");
        
    public:
        
        void setup();
        void update(float timeDelta);
        void draw();
        void got_message(const std::vector<uint8_t> &the_data);
        void tearDown();
        void updateProperty(const Property::ConstPtr &theProperty);
        
        void keyPress(const KeyEvent &e);
    };
}// namespace kinski

#endif /* defined(__gl__MovieTest__) */
