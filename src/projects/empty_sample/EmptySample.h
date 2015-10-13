//
//  EmptySample.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#ifndef __gl__EmptySample__
#define __gl__EmptySample__

#include "app/ViewerApp.h"

namespace kinski
{
    class EmptySample : public ViewerApp
    {
    private:
        
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
        void update_property(const Property::ConstPtr &theProperty) override;
    };
}// namespace kinski

#endif /* defined(__gl__EmptySample__) */
