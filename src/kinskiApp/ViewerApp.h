//
//  ViewerApp.h
//  kinskiGL
//
//  Created by Fabian on 3/1/13.
//
//

#ifndef __kinskiGL__ViewerApp__
#define __kinskiGL__ViewerApp__

#include "kinskiApp/GLFW_App.h"
#include "kinskiApp/LightComponent.h"
#include "kinskiApp/Arcball.h"
#include "kinskiGL/SerializerGL.h"
#include "kinskiGL/Scene.h"
#include "kinskiGL/Font.h"

namespace kinski {
    
    class ViewerApp : public GLFW_App
    {
    public:
        ViewerApp();
        virtual ~ViewerApp();
        
        void setup();
        void update(float timeDelta);
        void mousePress(const MouseEvent &e);
        void mouseDrag(const MouseEvent &e);
        void mouseRelease(const MouseEvent &e);
        void mouseWheel(const MouseEvent &e);
        void keyPress(const KeyEvent &e);
        void resize(int w, int h);
        
        // Property observer callback
        void updateProperty(const Property::ConstPtr &theProperty);
        
        Property_<std::vector<std::string> >::Ptr search_paths(){return m_search_paths;}
        bool wireframe() const { return *m_wireFrame; };
        void set_wireframe(bool b) { *m_wireFrame = b; };
        bool draw_grid() const { return *m_draw_grid; };
        bool normals() const { return *m_drawNormals; };
        const glm::vec3 light_direction(){ return -m_light_direction->value(); };
        void set_light_direction(const glm::vec3 &dir){ return m_light_direction->set(-dir); };
        const glm::vec4& clear_color(){ return *m_clear_color; };
        void clear_color(const gl::Color &the_color){ *m_clear_color = the_color; };
        void set_clear_color(const gl::Color &the_color){ *m_clear_color = the_color; };
        const gl::PerspectiveCamera::Ptr& camera() const { return m_camera; };
        gl::MeshPtr selected_mesh(){ return m_selected_mesh; };
        const std::vector<gl::MaterialPtr>& materials() const { return m_materials; };
        std::vector<gl::MaterialPtr>& materials(){ return m_materials; };
        std::vector<gl::LightPtr>& lights() { return m_lights; };
        const std::vector<gl::LightPtr>& lights() const { return m_lights; };
        const gl::Scene& scene() const { return m_scene; };
        gl::Scene& scene() { return m_scene; };
        bool precise_selection() const { return m_precise_selection; };
        void set_precise_selection(bool b){ m_precise_selection = b; };
        void set_camera(const gl::PerspectiveCamera::Ptr &theCam){m_camera = theCam;};
        
        virtual void save_settings(const std::string &path = "");
        virtual void load_settings(const std::string &path = "");
        
    private:
        std::vector<gl::MaterialPtr> m_materials;
        gl::MeshPtr m_selected_mesh;
        gl::PerspectiveCamera::Ptr m_camera;
        gl::Scene m_scene;
        gl::Arcball m_arcball;
        
        // Lightsources
        std::vector<gl::LightPtr> m_lights;
        
        bool m_precise_selection;
        bool m_center_selected;
        
        Property_<std::vector<std::string> >::Ptr m_search_paths;
        RangedProperty<int>::Ptr m_logger_severity;
        Property_<bool>::Ptr m_show_tweakbar;
        Property_<glm::vec2>::Ptr m_window_size;
        RangedProperty<float>::Ptr m_distance;
        Property_<glm::mat3>::Ptr m_rotation;
        RangedProperty<float>::Ptr m_rotationSpeed;
        Property_<bool>::Ptr m_draw_grid;
        Property_<bool>::Ptr m_wireFrame;
        Property_<bool>::Ptr m_drawNormals;
        Property_<glm::vec3>::Ptr m_light_direction;
        Property_<glm::vec4>::Ptr m_clear_color;
        
        // mouse rotation control
        glm::vec2 m_clickPos, m_dragPos, m_inertia;
        float m_rotation_damping;
        bool  m_mouse_down;
        glm::mat3 m_lastTransform;
        MovingAverage<glm::vec2> m_avg_filter;
    };
}// namespace


#endif /* defined(__kinskiGL__ViewerApp__) */
