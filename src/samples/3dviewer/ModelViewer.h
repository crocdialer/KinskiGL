//
//  3dViewer.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#ifndef __gl__3dViewer__
#define __gl__3dViewer__

#include "app/ViewerApp.h"
#include "app/LightComponent.h"

namespace kinski
{
    class ModelViewer : public ViewerApp
    {
    private:
        
        gl::MeshPtr m_mesh;
        
        LightComponent::Ptr m_light_component;
        
        Property_<std::string>::Ptr m_model_path = Property_<std::string>::create("Model path", "");
        
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
        void fileDrop(const MouseEvent &e, const std::vector<std::string> &files);
        void tearDown();
        void updateProperty(const Property::ConstPtr &theProperty);
    };
}// namespace kinski

#endif /* defined(__gl__3dViewer__) */
