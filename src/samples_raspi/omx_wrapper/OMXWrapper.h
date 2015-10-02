//
//  OMXWrapper.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#ifndef __gl__OMXWrapper__
#define __gl__OMXWrapper__

#include <thread> 
#include "app/ViewerApp.h"

namespace kinski
{
    class OMXWrapper : public ViewerApp
    {
    private:
        
        enum FontEnum{FONT_NORMAL = 0, FONT_LARGE = 1};

        Property_<bool>::Ptr 
        m_is_master = Property_<bool>::create("is master", false);
         
        Property_<string>::Ptr
        m_movie_path = Property_<string>::create("movie path", "~/tess_content/berge_01.mp4");
        
        Property_<float>::Ptr
        m_movie_delay = Property_<float>::create("movie start delay", 0.f);

        std::thread m_video_thread;
        void thread_func();

    public:
        
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
        void got_message(const std::vector<uint8_t> &the_message) override;
        void fileDrop(const MouseEvent &e, const std::vector<std::string> &files) override;
        void tearDown() override;
        void updateProperty(const Property::ConstPtr &theProperty) override;
    };
}// namespace kinski

#endif /* defined(__gl__OMXWrapper__) */
