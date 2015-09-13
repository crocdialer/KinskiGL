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
        
        enum ShaderEnum{SHADER_UNLIT = 0, SHADER_UNLIT_SKIN = 1, SHADER_PHONG = 2,
            SHADER_PHONG_SKIN = 3, SHADER_PHONG_SHADOWS = 4, SHADER_PHONG_SKIN_SHADOWS = 5};
        
        std::map<ShaderEnum, gl::Shader> m_shaders;
        
        gl::MeshPtr m_mesh;
        gl::Texture m_cube_map;
        
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
