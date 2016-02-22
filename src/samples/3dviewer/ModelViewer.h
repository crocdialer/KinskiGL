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

namespace kinski
{
    class ModelViewer : public ViewerApp
    {
    private:
        
        gl::MeshPtr m_mesh;
        gl::Texture m_cube_map;
        
        bool m_dirty_shader = true;
        
        Property_<bool>::Ptr
        m_use_lighting = Property_<bool>::create("use lighting", true),
        m_use_ground_plane = Property_<bool>::create("use ground plane", true);
        
        Property_<std::string>::Ptr
        m_model_path = Property_<std::string>::create("Model path", ""),
        m_cube_map_folder = Property_<std::string>::create("Cubemap folder", "");
        
        Property_<bool>::Ptr
        m_use_bones = Property_<bool>::create("use bones", true);
        
        Property_<bool>::Ptr
        m_display_bones = Property_<bool>::create("display bones", false);
        
        Property_<uint32_t>::Ptr
        m_animation_index = Property_<uint32_t>::create("animation index", 0);
        
        RangedProperty<float>::Ptr
        m_animation_speed = RangedProperty<float>::create("animation speed", 1.f, -1.5f, 1.5f);
        
        void build_skeleton(gl::BonePtr currentBone, vector<gl::vec3> &points,
                            vector<string> &bone_names);
        
        bool load_asset(const std::string &the_path);
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

#endif /* defined(__gl__3dViewer__) */
