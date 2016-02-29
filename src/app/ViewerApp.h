//
//  ViewerApp.h
//  gl
//
//  Created by Fabian on 3/1/13.
//
//

#pragma once

#include "core/Timer.hpp"
#include "core/Animation.hpp"
#include "core/Measurement.hpp"
#include "app/Arcball.h"
#include "app/RemoteControl.h"
#include "app/LightComponent.h"
#include "gl/SerializerGL.hpp"
#include "gl/Scene.hpp"
#include "gl/Fbo.hpp"
#include "gl/Font.hpp"

#if defined(KINSKI_RASPI)
    #include "app/Raspi_App.h"
    #define BaseApp Raspi_App
#else
    #include "app/GLFW_App.h"
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
        void mousePress(const MouseEvent &e) override;
        void mouseDrag(const MouseEvent &e) override;
        void mouseRelease(const MouseEvent &e) override;
        void mouseWheel(const MouseEvent &e) override;
        void keyPress(const KeyEvent &e) override;
        void resize(int w, int h) override;
        
        // Property observer callback
        void update_property(const Property::ConstPtr &theProperty) override;
        
        Property_<std::vector<std::string> >::Ptr search_paths(){return m_search_paths;}
        bool wireframe() const { return *m_wireframe; };
        void set_wireframe(bool b) { *m_wireframe = b; };
        bool draw_grid() const { return *m_draw_grid; };
        bool normals() const { return *m_drawNormals; };
        const glm::vec4& clear_color(){ return *m_clear_color; };
        void clear_color(const gl::Color &the_color){ *m_clear_color = the_color; };
        void set_clear_color(const gl::Color &the_color){ *m_clear_color = the_color; };
        
        const gl::PerspectiveCamera::Ptr& camera() const { return m_camera; };
        const gl::OrthographicCamera::Ptr& gui_camera() const { return m_gui_camera; };
        
        const gl::MeshPtr& selected_mesh() const { return m_selected_mesh; };
        void set_selected_mesh(gl::MeshPtr m){ m_selected_mesh = m; };
        const std::vector<gl::MaterialPtr>& materials() const { return m_materials; };
        std::vector<gl::MaterialPtr>& materials(){ return m_materials; };
        
        std::vector<gl::LightPtr>& lights() { return m_lights; };
        const std::vector<gl::LightPtr>& lights() const { return m_lights; };
        
        std::vector<gl::Texture>& textures() { return m_textures; };
        const std::vector<gl::Texture>& textures() const { return m_textures; };
        
        std::vector<gl::Font>& fonts() { return m_fonts; };
        const std::vector<gl::Font>& fonts() const { return m_fonts; };
        
        std::vector<animation::AnimationPtr>& animations() { return m_animations; };
        const std::vector<animation::AnimationPtr>& animations() const { return m_animations; };
        
        const gl::Scene& scene() const { return m_scene; };
        gl::Scene& scene() { return m_scene; };
        bool precise_selection() const { return m_precise_selection; };
        void set_precise_selection(bool b){ m_precise_selection = b; };
        void set_camera(const gl::PerspectiveCamera::Ptr &theCam){m_camera = theCam;};
        uint32_t cam_index() const { return m_cam_index; }
        
        virtual bool save_settings(const std::string &path = "");
        virtual bool load_settings(const std::string &path = "");
        
        void draw_textures(const std::vector<gl::Texture> &the_textures);
        
        gl::Texture generate_snapshot();
        gl::Texture& snapshot_texture(){ return m_snapshot_texture; }
        
    protected:
        
        std::vector<gl::Font> m_fonts{4};
        
        std::vector<gl::MaterialPtr> m_materials;
        gl::MeshPtr m_selected_mesh;
        
        gl::PerspectiveCamera::Ptr m_camera;
        uint32_t m_cam_index;
        
        gl::Scene m_scene;
        gl::Arcball m_arcball;
        
        gl::OrthographicCamera::Ptr m_gui_camera;
        
        // Lightsources
        std::vector<gl::LightPtr> m_lights;
        
        std::vector<gl::Texture> m_textures {16};
        gl::Texture m_snapshot_texture;
        
        std::vector<animation::AnimationPtr> m_animations{8};
        
        bool m_precise_selection;
        bool m_center_selected;
        glm::vec3 m_look_at_tmp;
        
        gl::Fbo m_fbo_snapshot;
        
        Property_<std::vector<std::string> >::Ptr m_search_paths;
        RangedProperty<int>::Ptr m_logger_severity;
        Property_<bool>::Ptr m_show_tweakbar;
        Property_<glm::vec2>::Ptr m_window_size;
        
        
        RangedProperty<float>::Ptr m_distance;
        Property_<glm::mat3>::Ptr m_rotation;
        Property_<float>::Ptr m_camera_fov;
        Property_<glm::vec3>::Ptr m_look_at;
        
        RangedProperty<float>::Ptr m_rotation_speed;
        Property_<glm::vec3>::Ptr m_rotation_axis;
        Property_<bool>::Ptr m_draw_grid;
        Property_<bool>::Ptr m_wireframe;
        Property_<bool>::Ptr m_drawNormals;
        Property_<glm::vec4>::Ptr m_clear_color;
        
        // mouse rotation control
        glm::vec2 m_clickPos, m_dragPos, m_inertia;
        float m_rotation_damping;
        bool  m_mouse_down;
        glm::mat3 m_lastTransform;
        MovingAverage<glm::vec2> m_avg_filter;
        
        // control module for light objects
        LightComponent::Ptr m_light_component;
        
        // tcp remote control
        RemoteControl m_remote_control;
    };
}// namespace