// ImGui GLFW binding with OpenGL3 + shaders
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)
// (GL3W is a helper library to access OpenGL functions since there is no standard header to access modern OpenGL functions easily. Alternatives are GLEW, Glad, etc.)

// Implemented features:
//  [X] User texture binding. Cast 'GLuint' OpenGL texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.
//  [X] Gamepad navigation mapping. Enable with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include "app/App.hpp"
#include "imgui.h"

namespace kinski{ namespace gui {

KINSKI_API bool init(kinski::App *the_app);

KINSKI_API void shutdown();

KINSKI_API void new_frame();

KINSKI_API void end_frame();

KINSKI_API void render();

// Use if you want to reset your rendering device without losing ImGui state.
KINSKI_API void invalidate_device_objects();

KINSKI_API bool create_device_objects();

KINSKI_API void mouse_press(const MouseEvent &e);
//KINSKI_API void mouse_release(const MouseEvent &e);
KINSKI_API void mouse_wheel(const MouseEvent &e);

KINSKI_API void key_press(const KeyEvent &e);
KINSKI_API void key_release(const KeyEvent &e);
KINSKI_API void char_callback(uint32_t c);

}}// namespace