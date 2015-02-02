//
//  FractureApp.h
//  gl
//
//  Created by Fabian on 01/02/15.
//
//

#ifndef __gl__3dViewer__
#define __gl__3dViewer__

#include "app/ViewerApp.h"
#include "app/LightComponent.h"

// module header
#include "physics_context.h"

namespace kinski
{
    class FractureApp : public ViewerApp
    {
    private:
        
        gl::MeshPtr m_mesh;
        
        physics::physics_context m_physics;
        
        LightComponent::Ptr m_light_component;
        
        Property_<std::string>::Ptr m_model_path = Property_<std::string>::create("Model path", "");
        
        void shoot_box(const glm::vec3 &the_half_extents = glm::vec3(1));
        
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

#endif /* defined(__gl__3dViewer__) */
