//
//  Feldkirsche_App.h
//  kinskiGL
//
//  Created by Fabian on 6/17/13.
//
//

#ifndef __kinskiGL__Feldkirsche_App__
#define __kinskiGL__Feldkirsche_App__

#include "kinskiApp/ViewerApp.h"

#include "kinskiApp/LightComponent.h"

// offscreen rendering
#include "kinskiGL/Fbo.h"

// model loading
#include "AssimpConnector.h"

// physics
#include "physics_context.h"

// Syphon
#include "SyphonConnector.h"

// Enviroment mapping
#include "CubeMap.h"

namespace kinski{
    
class Feldkirsche_App : public ViewerApp
{
private:
    
    std::vector<gl::Texture> m_textures;
    gl::MaterialPtr m_material;
    gl::MeshPtr m_mesh, m_spot_mesh;
    gl::Font m_font;
    
    Property_<std::string>::Ptr m_modelPath;
    Property_<glm::vec3>::Ptr m_modelScale;
    Property_<glm::vec3>::Ptr m_modelOffset;
    Property_<float>::Ptr m_modelRotationY;
    Property_<glm::vec4>::Ptr m_color;
    Property_<float>::Ptr m_shinyness;
    RangedProperty<float>::Ptr m_reflection;
    
    // physics
    Property_<bool>::Ptr m_stepPhysics;
    kinski::physics::physics_context m_physics_context;
    std::shared_ptr<kinski::physics::BulletDebugDrawer> m_debugDrawer;
    btRigidBody *m_ground_body, *m_left_body, *m_right_body;
    RangedProperty<int>::Ptr m_rigid_bodies_num;
    RangedProperty<float>::Ptr m_rigid_bodies_size;
    Property_<glm::vec3>::Ptr m_gravity;
    Property_<float>::Ptr m_gravity_amount;
    Property_<float>::Ptr m_gravity_smooth;
    RangedProperty<float>::Ptr m_gravity_max_roll;
    
    Property_<glm::vec3>::Ptr m_world_half_extents;
    std::list<physics::physics_object> m_water_objects;
    gl::MaterialPtr m_water_materials[4];
    
    // offscreen rendering
    enum DRAW_MODE{DRAW_FBO_OUTPUT = 0, DRAW_DEBUG_SCENE = 1};
    RangedProperty<int>::Ptr m_debug_draw_mode;
    gl::PerspectiveCamera::Ptr m_free_camera;
    gl::MeshPtr m_free_camera_mesh;
    gl::Fbo m_fbo, m_mask_fbo, m_result_fbo;
    Property_<glm::vec2>::Ptr m_fbo_size;
    RangedProperty<float>::Ptr m_fbo_cam_distance;
    RangedProperty<float>::Ptr m_fbo_cam_fov;
    Property_<glm::mat4>::Ptr m_fbo_cam_transform;
    Property_<glm::vec3>::Ptr m_fbo_cam_direction;
    
    // output via Syphon
    gl::SyphonConnector m_syphon;
    Property_<bool>::Ptr m_use_syphon;
    Property_<std::string>::Ptr m_syphon_server_name;
    
    
    // path for a optional shader to use
    Property_<std::vector<std::string> >::Ptr m_custom_shader_paths;
    gl::Shader m_custom_shader;
    
    // contains debug-view objects
    gl::Scene m_debug_scene;
    
    // Light control
    LightComponent::Ptr m_light_component;
    gl::LightPtr m_spot_light;
    
public:
    
    void setup();
    void update(float timeDelta);
    void draw();
    void tearDown();
    
    virtual void save_settings(const std::string &path = "");
    virtual void load_settings(const std::string &path = "");
    
    void mousePress(const MouseEvent &e);
    void keyPress(const KeyEvent &e);
    void keyRelease(const KeyEvent &e);
    
    //! Property observer callback
    void updateProperty(const Property::ConstPtr &theProperty);
    
    //! add a mesh to our scene and physics-world
    void add_mesh(gl::MeshPtr the_mesh, glm::vec3 scale = glm::vec3(1.f), bool use_offsets = true);
    
    //! add a mesh to our physics-world only
    void add_mesh_to_simulation(gl::MeshPtr the_mesh, glm::vec3 scale = glm::vec3(1.f));
    
    //! reset scene and create a stack of rigidbodys
    void create_physics_scene(int size_x, int size_y, int size_z, const gl::MaterialPtr &theMat);
    
};
}//namespace

#endif /* defined(__kinskiGL__Feldkirsche_App__) */