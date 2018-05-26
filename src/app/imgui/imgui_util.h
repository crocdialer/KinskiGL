//
// Created by crocdialer on 4/20/18.
//

#ifndef KINSKIGL_IMGUI_UTIL_H
#define KINSKIGL_IMGUI_UTIL_H

#include "core/Component.hpp"
#include "gl/gl.hpp"
#include "app/App.hpp"
#include "imgui.h"

// forward declared to avoid inclusion of App.h
class JoystickState;

namespace kinski{ namespace gui{

const ImVec2& im_vec_cast(const gl::vec2 &the_vec);
const ImVec4& im_vec_cast(const gl::vec4 &the_vec);
const ImVec4 im_vec_cast(const gl::vec3 &the_vec);

//! draw a kinski::Component using ImGui
void draw_component_ui(const ComponentConstPtr &the_component);

void draw_textures_ui(const std::vector<gl::Texture> &the_textures);

void draw_lights_ui(const std::vector<gl::LightPtr> &the_lights);

void draw_material_ui(const gl::MaterialPtr &the_mat);
void draw_materials_ui(const std::vector<gl::MaterialPtr> &the_materials);

void draw_mesh_ui(const gl::MeshPtr &the_mesh);

void process_joystick_input(const std::vector<JoystickState> &the_joystick_states);

}}// namespaces

#endif //KINSKIGL_IMGUI_UTIL_H
