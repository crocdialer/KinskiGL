//
// Created by crocdialer on 4/20/18.
//

#ifndef KINSKIGL_IMGUI_UTIL_H
#define KINSKIGL_IMGUI_UTIL_H

#include "core/Component.hpp"
#include "imgui.h"

namespace kinski{ namespace gl{

//! draw a kinski::Component using ImGui
void draw_component_ui(const ComponentConstPtr &the_component);

}}// namespaces

#endif //KINSKIGL_IMGUI_UTIL_H
