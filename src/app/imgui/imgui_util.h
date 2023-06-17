//
// Created by crocdialer on 4/20/18.
//

#ifndef KINSKIGL_IMGUI_UTIL_H
#define KINSKIGL_IMGUI_UTIL_H

#include "app/App.hpp"
#include "app/Component.hpp"
#include "app/LightComponent.hpp"
#include "app/WarpComponent.hpp"
#include "gl/gl.hpp"
#include "imgui.h"

// forward declared to avoid inclusion of App.h
class JoystickState;

namespace kinski{ namespace gui{

const ImVec2& im_vec_cast(const gl::vec2 &the_vec);
const ImVec4& im_vec_cast(const gl::vec4 &the_vec);
const ImVec4 im_vec_cast(const gl::vec3 &the_vec);

//! draw a generic kinski::Component using ImGui
void draw_component_ui(const crocore::ComponentConstPtr &the_component);

void draw_textures_ui(const std::vector<gl::Texture*> &the_textures);

void draw_material_ui(const gl::MaterialPtr &the_mat);
void draw_materials_ui(const std::vector<gl::MaterialPtr> &the_materials);

void draw_light_component_ui(const LightComponentPtr &the_component);

void draw_warp_component_ui(const WarpComponentPtr &the_component);

void draw_object3D_ui(const gl::Object3DPtr &the_object,
                      const gl::CameraConstPtr &the_camera = nullptr);

void draw_mesh_ui(const gl::MeshPtr &the_mesh);

void draw_scenegraph_ui(const gl::SceneConstPtr &the_scene, std::set<gl::Object3DPtr>* the_selection = nullptr);

void process_joystick_input(const std::vector<JoystickState> &the_joystick_states);

}}// namespaces

#endif //KINSKIGL_IMGUI_UTIL_H
