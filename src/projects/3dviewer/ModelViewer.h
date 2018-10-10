// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  3dViewer.hpp
//
//  Created by Fabian Schmidt on 29/01/14.

#pragma once

#include "app/ViewerApp.hpp"
#include "gl/DeferredRenderer.hpp"

namespace kinski
{
    class ModelViewer : public ViewerApp
    {
    private:
        
        enum TextureEnum{ TEXTURE_OFFSCREEN = 0, TEXTURE_OUTPUT = 1 };

        gl::DeferredRendererPtr m_deferred_renderer = gl::DeferredRenderer::create();

        gl::MeshPtr m_load_indicator;
        gl::Texture m_normal_map;
        
        gl::FboPtr m_post_process_fbo, m_offscreen_fbo;
        gl::MaterialPtr m_post_process_mat;
        
        Property_<float>::Ptr
        m_focal_length = Property_<float>::create("focal length", 200.f),
        m_focal_depth = RangedProperty<float>::create("focal depth", 50.f, 0.f, 200.f),
        m_circle_of_confusion_sz = Property_<float>::create("circle of confusion size", 0.03f),
        m_fstop = RangedProperty<float>::create("fstop", 1.f, 0.f, 200.f),
        m_gain = RangedProperty<float>::create("gain", 1.f, 0.f, 20.f),
        m_fringe = RangedProperty<float>::create("fringe", .7f, 0.f, 10.f),
        m_ground_plane_texture_scale = RangedProperty<float>::create("groundplane texture scale", 1.f, -5.f, 5.f),
        m_enviroment_strength = RangedProperty<float>::create("enviroment strength", 1.f, 0.f, 4.f);
        
        Property_<bool>::Ptr
        m_debug_focus = Property_<bool>::create("debug focus", false),
        m_auto_focus = Property_<bool>::create("auto focus", false),
        m_use_post_process = Property_<bool>::create("use post process", false),
        m_use_fxaa = Property_<bool>::create("use FXAA", true);
        
        Property_<gl::vec2>::Ptr
        m_offscreen_resolution = Property_<gl::vec2>::create("offscreen resolution", gl::vec2(0));
        
        bool m_dirty_shader = true;
        bool m_dirty_g_buffer = true;
        
        Property_<bool>::Ptr
        m_draw_fps = Property_<bool>::create("draw fps", true),
        m_use_deferred_render = Property_<bool>::create("use deferred rendering", true),
        m_use_lighting = Property_<bool>::create("use lighting", true),
        m_use_normal_map = Property_<bool>::create("use normal mapping", true),
        m_use_ground_plane = Property_<bool>::create("use ground plane", true);
        
        Property_<std::string>::Ptr
        m_model_path = Property_<std::string>::create("Model path", ""),
        m_normalmap_path = Property_<std::string>::create("normalmap path", ""),
        m_skybox_path = Property_<std::string>::create("skybox path", "");

        Property_<std::vector<std::string>>::Ptr
        m_ground_textures = Property_<std::vector<std::string>>::create("ground textures");
        
        Property_<bool>::Ptr
        m_use_bones = Property_<bool>::create("use bones", true);
        
        Property_<bool>::Ptr
        m_display_bones = Property_<bool>::create("display bones", false);

        Property_<gl::vec2>::Ptr
        m_blur_amount = Property_<gl::vec2>::create("blur amount", gl::vec2(1.f));

        void build_skeleton(gl::BonePtr currentBone, const glm::mat4 start_transform,
                            vector<gl::vec3> &points, vector<string> &bone_names);
        
        //! asset loading routine
        gl::MeshPtr load_asset(const std::string &the_path);
        
        //! asynchronous asset loading
        void async_load_asset(const std::string &the_path,
                              std::function<void(gl::MeshPtr)> the_completion_handler);
        
        void update_shader();
        
        void render_bones(const gl::MeshPtr &the_mesh, const gl::CameraPtr &the_cam, bool use_labels);
        
        void process_joystick(float the_time_delta);

        void draw_load_indicator(const gl::vec2 &the_screen_pos, float the_size);

    public:
        
        ModelViewer(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
        void setup() override;
        void update(float timeDelta) override;
        void draw() override;
        void resize(int w ,int h) override;
        void key_press(const KeyEvent &e) override;
        void key_release(const KeyEvent &e) override;
        void mouse_press(const MouseEvent &e) override;
        void mouse_release(const MouseEvent &e) override;
        void mouse_move(const MouseEvent &e) override;
        void mouse_drag(const MouseEvent &e) override;
        void mouse_wheel(const MouseEvent &e) override;
        void touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void file_drop(const MouseEvent &e, const std::vector<std::string> &files) override;
        void teardown() override;
        void update_property(const Property::ConstPtr &theProperty) override;

        void update_fbos();
    };
}// namespace kinski

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::ModelViewer>(argc, argv);
    return theApp->run();
}
