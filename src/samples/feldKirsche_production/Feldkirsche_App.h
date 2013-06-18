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
#include "kinskiGL/Fbo.h"

// model loading
#include "AssimpConnector.h"

// physics
#include "physics_context.h"

// Syphon
#include "SyphonConnector.h"

// OpenNI
#include "OpenNIConnector.h"

namespace kinski{
    
class Feldkirsche_App : public ViewerApp
{
private:
    
    std::vector<gl::Texture> m_textures;
    gl::MaterialPtr m_material;
    gl::MeshPtr m_mesh;
    gl::Font m_font;
    
    Property_<std::string>::Ptr m_modelPath;
    Property_<glm::vec3>::Ptr m_modelScale;
    Property_<glm::vec3>::Ptr m_modelOffset;
    Property_<glm::vec4>::Ptr m_color;
    Property_<float>::Ptr m_shinyness;
    
    // physics
    Property_<bool>::Ptr m_stepPhysics;
    kinski::physics::physics_context m_physics_context;
    std::shared_ptr<kinski::physics::BulletDebugDrawer> m_debugDrawer;
    btRigidBody *m_ground_body, *m_left_body, *m_right_body;
    RangedProperty<int>::Ptr m_rigid_bodies_num;
    RangedProperty<float>::Ptr m_rigid_bodies_size;
    Property_<glm::vec3>::Ptr m_gravity;
    Property_<glm::vec3>::Ptr m_world_half_extents;
    
    // offscreen rendering
    enum DRAW_MODE{DRAW_FBO_OUTPUT = 0, DRAW_DEBUG_SCENE = 1};
    RangedProperty<int>::Ptr m_debug_draw_mode;
    gl::PerspectiveCamera::Ptr m_free_camera;
    gl::MeshPtr m_free_camera_mesh;
    gl::Fbo m_fbo;
    Property_<glm::vec2>::Ptr m_fbo_size;
    RangedProperty<float>::Ptr m_fbo_cam_distance;
    Property_<glm::mat4>::Ptr m_fbo_cam_transform;
    
    // output via Syphon
    gl::SyphonConnector m_syphon;
    Property_<bool>::Ptr m_use_syphon;
    Property_<std::string>::Ptr m_syphon_server_name;
    
    // OpenNI interface / user interaction
    gl::OpenNIConnector::Ptr m_open_ni;
    gl::OpenNIConnector::UserList m_user_list;
    gl::MeshPtr m_depth_cam_mesh, m_user_mesh, m_user_radius_mesh;
    std::vector<gl::Color> m_user_id_colors;
    gl::PerspectiveCamera::Ptr m_depth_cam;
    RangedProperty<float>::Ptr m_depth_cam_x, m_depth_cam_y, m_depth_cam_z;
    Property_<glm::vec3>::Ptr m_depth_cam_look_dir;
    RangedProperty<float>::Ptr m_depth_cam_scale;
    RangedProperty<float>::Ptr m_min_interaction_distance, m_user_offset;
    
    // contains debug-view objects
    gl::Scene m_debug_scene;
    
public:
    
    void setup();
    void update(float timeDelta);
    void draw();
    void tearDown();
    
    void mousePress(const MouseEvent &e);
    void keyPress(const KeyEvent &e);
    void keyRelease(const KeyEvent &e);
    
    //! Property observer callback
    void updateProperty(const Property::ConstPtr &theProperty);
    
    //! add a mesh to our scene and physics-world
    void add_mesh(const gl::MeshPtr &the_mesh, glm::vec3 scale = glm::vec3(1.f));
    
    //! reset scene and create a stack of rigidbodys
    void create_physics_scene(int size_x, int size_y, int size_z, const gl::MaterialPtr &theMat);
    
    //! bring depth-cam-relative positions to world-coords using a virtual camera
    void adjust_user_positions_with_camera(gl::OpenNIConnector::UserList &user_list,
                                           const gl::CameraPtr &cam);
    
    //! draws a list of Users as bounding spheres
    void draw_user_meshes(const gl::OpenNIConnector::UserList &user_list);
    
    void update_gravity(const gl::OpenNIConnector::UserList &user_list, float factor);
};
}//namespace

#endif /* defined(__kinskiGL__Feldkirsche_App__) */