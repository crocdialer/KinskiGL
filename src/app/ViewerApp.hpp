// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  ViewerApp.hpp
//
//  Created by Fabian on 3/1/13.

#pragma once

#include <crocore/Timer.hpp>
#include <crocore/CircularBuffer.hpp>

#include "app/RemoteControl.hpp"
#include "app/LightComponent.hpp"
#include "app/WarpComponent.hpp"
#include "gl/SerializerGL.hpp"
#include "gl/Scene.hpp"
#include "gl/Fbo.hpp"
#include "gl/Font.hpp"

#if defined(KINSKI_ARM)
    #include "app/EGL_App.hpp"
    #define BaseApp EGL_App
#else
    #include "app/GLFW_App.hpp"
    #define BaseApp GLFW_App
#endif

namespace kinski {
    
    class ViewerApp : public BaseApp
    {
    public:
        ViewerApp(int argc = 0, char *argv[] = nullptr);
        virtual ~ViewerApp();
        
        void setup() override;
        void update(float timeDelta) override;
        void mouse_press(const MouseEvent &e) override;
        void mouse_release(const MouseEvent &e) override;
        void mouse_move(const MouseEvent &e) override;
        void mouse_drag(const MouseEvent &e) override;
        void mouse_wheel(const MouseEvent &e) override;
        void key_press(const KeyEvent &e) override;
        void key_release(const KeyEvent &e) override;
        void resize(int w, int h) override;
        
        // Property observer callback
        void update_property(const crocore::PropertyConstPtr &theProperty) override;

        crocore::Property_<std::vector<std::string> >::Ptr search_paths(){return m_search_paths;}
        bool draw_grid() const { return *m_draw_grid; };
        const gl::Color& clear_color(){ return *m_clear_color; };
        void clear_color(const gl::Color &the_color){ *m_clear_color = the_color; };
        void set_clear_color(const gl::Color &the_color){ *m_clear_color = the_color; };
        
        const gl::PerspectiveCamera::Ptr& camera() const { return m_camera; };
        const gl::OrthoCamera::Ptr& gui_camera() const { return m_gui_camera; };
        
        const std::set<gl::Object3DPtr>& selected_objects() const{ return m_selected_objects; };
        std::set<gl::Object3DPtr>& selected_objects(){ return m_selected_objects; };
        void add_selected_object(const gl::Object3DPtr& the_obj);
        void remove_selected_object(const gl::Object3DPtr& the_obj);
        void clear_selected_objects();
        
        std::vector<gl::LightPtr>& lights() { return m_lights; };
        const std::vector<gl::LightPtr>& lights() const { return m_lights; };
        
        std::vector<gl::Texture>& textures() { return m_textures; };
        const std::vector<gl::Texture>& textures() const { return m_textures; };
        
        std::vector<gl::Font>& fonts() { return m_fonts; };
        const std::vector<gl::Font>& fonts() const { return m_fonts; };
        
        const gl::ScenePtr& scene() const { return m_scene; };
        gl::ScenePtr& scene() { return m_scene; };
        bool precise_selection() const { return m_precise_selection; };
        void set_precise_selection(bool b){ m_precise_selection = b; };
        void set_camera(const gl::PerspectiveCamera::Ptr &theCam){m_camera = theCam;};
        uint32_t cam_index() const { return m_cam_index; }
        
        virtual bool save_settings(const std::string &path = "");
        virtual bool load_settings(const std::string &path = "");
        
        const std::string& default_config_path() const { return m_default_config_path; }
        void set_default_config_path(const std::string& the_path) { m_default_config_path = the_path; }
        
        void draw_textures(const std::vector<gl::Texture> &the_textures);
        
        void async_load_texture(const std::string &the_path,
                                std::function<void(const gl::Texture&)> the_callback,
                                bool mip_map = false,
                                bool compress = false,
                                GLfloat anisotropic_filter_lvl = 1.f);
        
        gl::Texture generate_snapshot();
        gl::Texture& snapshot_texture(){ return m_snapshot_texture; }
        const gl::FboPtr& snapshot_fbo(){ return m_snapshot_fbo; }
        
        RemoteControl& remote_control(){ return m_remote_control; }
        
    protected:
        
        std::vector<gl::Font> m_fonts{4};
        
        std::vector<gl::MaterialPtr> m_materials;
        std::set<gl::Object3DPtr> m_selected_objects;
        
        gl::PerspectiveCamera::Ptr m_camera;
        uint32_t m_cam_index;
        
        gl::ScenePtr m_scene;
        
        gl::OrthoCamera::Ptr m_gui_camera;
        
        // Lightsources
        std::vector<gl::LightPtr> m_lights;
        
        std::vector<gl::Texture> m_textures {16};
        gl::Texture m_snapshot_texture;
        
        bool m_precise_selection;
        bool m_center_selected;
        glm::vec3 m_look_at_tmp;
        
        gl::FboPtr m_snapshot_fbo;
        
        std::string m_default_config_path = "./";

        crocore::Property_<std::vector<std::string> >::Ptr m_search_paths;
        crocore::RangedProperty<int>::Ptr m_logger_severity;
        crocore::Property_<bool>::Ptr m_show_tweakbar;
        crocore::Property_<bool>::Ptr m_hide_cursor;
        crocore::Property_<glm::vec2>::Ptr m_window_size;
        crocore::Property_<bool>::Ptr m_fullscreen;
        crocore::Property_<bool>::Ptr m_v_sync;

        bool m_dirty_cam = true;
        crocore::RangedProperty<float>::Ptr m_distance;
        crocore::Property_<glm::mat3>::Ptr m_rotation;
        crocore::Property_<float>::Ptr m_camera_fov;
        crocore::Property_<glm::vec3>::Ptr m_look_at;

        crocore::RangedProperty<float>::Ptr m_rotation_speed;
        crocore::Property_<glm::vec3>::Ptr m_rotation_axis;
        crocore::Property_<bool>::Ptr m_draw_grid;
        crocore::Property_<bool>::Ptr m_use_warping;
        crocore::Property_<gl::Color>::Ptr m_clear_color;
        
        // mouse rotation control
        glm::vec2 m_clickPos, m_dragPos, m_inertia;
        float m_rotation_damping;
        bool m_mouse_down;
        glm::mat3 m_lastTransform;
        crocore::CircularBuffer<gl::vec2> m_drag_buffer;
        
        // control module for light objects
        LightComponentPtr m_light_component;
        
        // control module for quad warping
        WarpComponentPtr m_warp_component;
        
        // tcp remote control
        RemoteControl m_remote_control;
    };
}// namespace
