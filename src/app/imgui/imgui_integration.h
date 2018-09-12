#include "app/App.hpp"
#include "imgui.h"
#include "ImGuizmo.h"

namespace kinski{ namespace gui {

KINSKI_API bool init(kinski::App *the_app);

KINSKI_API void shutdown();

KINSKI_API void new_frame();

KINSKI_API void end_frame();

KINSKI_API void render();

KINSKI_API void invalidate_device_objects();

KINSKI_API bool create_device_objects();

KINSKI_API void mouse_press(const MouseEvent &e);
KINSKI_API void mouse_wheel(const MouseEvent &e);

KINSKI_API void key_press(const KeyEvent &e);
KINSKI_API void key_release(const KeyEvent &e);
KINSKI_API void char_callback(uint32_t c);

}}// namespace