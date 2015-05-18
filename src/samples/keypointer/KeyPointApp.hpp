//
//  EmptySample.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#ifndef __gl__KeyPointApp__
#define __gl__KeyPointApp__

#include "app/ViewerApp.h"
#include "cv/CVThread.h"
#include "cv/TextureIO.h"
#include "KeyPointNode.h"

namespace kinski
{
    class KeyPointApp : public ViewerApp
    {
    private:
        
        Property_<bool>::Ptr
        m_activator = Property_<bool>::create("processing", true);
        
        Property_<std::string>::Ptr
        m_img_path = Property_<std::string>::create("image path", "kinder.jpg");
        
        RangedProperty<uint32_t>::Ptr
        m_imageIndex = RangedProperty<uint32_t>::create("Image Index", 2, 0, 2);
        
        CVThread::Ptr m_cvThread;
        CVProcessNode::Ptr m_processNode;
        
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

#endif /* defined(__gl__KeyPointApp__) */
