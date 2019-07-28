#include "app/App.hpp"
#include "imgui.h"
#include "ImGuizmo.h"

namespace kinski{ namespace gui {

const ImVec2& im_vec_cast(const glm::vec2 &the_vec);
const ImVec4& im_vec_cast(const glm::vec4 &the_vec);
const ImVec4 im_vec_cast(const glm::vec3 &the_vec);

bool init(kinski::App *the_app);

void shutdown();

void new_frame();

void end_frame();

void render();

void invalidate_device_objects();

bool create_device_objects();

void mouse_press(const MouseEvent &e);
void mouse_wheel(const MouseEvent &e);

void key_press(const KeyEvent &e);
void key_release(const KeyEvent &e);
void char_callback(uint32_t c);

}}// namespace