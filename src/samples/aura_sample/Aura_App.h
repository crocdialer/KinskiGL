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

// audio
#include "Fmod_Sound.h"

#include <boost/shared_array.hpp>

namespace kinski{
    
class Aura_App : public ViewerApp
{
private:
    
    std::vector<gl::Texture> m_textures;
    gl::MaterialPtr m_material;
    gl::MeshPtr m_mesh;
    gl::Font m_font;
    
    Property_<std::string>::Ptr m_modelPath;
    Property_<glm::vec3>::Ptr m_modelScale;
    Property_<glm::vec3>::Ptr m_modelOffset;
    Property_<float>::Ptr m_modelRotationY;
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
    
    // path for a optional shader to use
    Property_<std::vector<std::string> >::Ptr m_custom_shader_paths;
    gl::Shader m_custom_shader;
    
    // contains debug-view objects
    gl::Scene m_debug_scene;
    
    // test mesh creation
    RangedProperty<int>::Ptr m_num_vertices;
    
    
    gl::MeshPtr m_fancy_line_mesh;
    //gl::Shader m_line_shader;
    
    gl::MeshPtr create_fancy_cube(int num_vertices);
    gl::MeshPtr create_fancy_lines(int num_x, int num_y, const glm::vec2 &step = glm::vec2(10.f),
                                   const glm::vec3 &noise_params = glm::vec3(0.025));
    gl::MeshPtr create_fancy_ufo(float radius, int num_rings);
    
    kinski::Component::Ptr m_noise_component;
    
    audio::SoundPtr m_test_sound;
    boost::shared_array<float> m_fft_smoothed;
    
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
};
}//namespace

#endif /* defined(__kinskiGL__Feldkirsche_App__) */