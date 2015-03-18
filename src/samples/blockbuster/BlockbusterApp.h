//
//  BlockbusterApp.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#ifndef __gl__BlockbusterApp__
#define __gl__BlockbusterApp__

#include "app/ViewerApp.h"
#include "app/LightComponent.h"

namespace kinski
{
    class BlockbusterApp : public ViewerApp
    {
    private:
        
        LightComponent::Ptr m_light_component;
        
        gl::MeshPtr m_mesh;
        std::vector<glm::vec3> m_user_positions;
        
        gl::MeshPtr create_mesh();
        glm::vec3 click_pos_on_ground(const glm::vec2 click_pos);
        
        Property_<uint32_t>::Ptr
        m_num_tiles_x = Property_<uint32_t>::create("num tiles x", 16),
        m_num_tiles_y = Property_<uint32_t>::create("num tiles y", 10);
        
        bool m_dirty = true;
        
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

#endif /* defined(__gl__BlockbusterApp__) */
