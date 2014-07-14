//
//  NoiseSample.h
//  kinskiGL
//
//  Created by Fabian on 29/01/14.
//
//

#ifndef __kinskiGL__NoiseSample__
#define __kinskiGL__NoiseSample__

// Audio
#include "RtAudio.h"

#include "kinskiApp/ViewerApp.h"

namespace kinski
{
    class NoiseSample : public ViewerApp
    {
    private:
        
        gl::Font m_font;
        
        // audio system
        RtAudio m_audio;
        
        bool m_streaming = false;
        
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

#endif /* defined(__kinskiGL__NoiseSample__) */
