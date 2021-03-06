#include <sys/time.h>
#include <regex.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include "esUtil.h"
#undef countof

#include <crocore/filesystem.hpp>
#include "app/imgui/imgui_integration.h"
#include "EGL_App.hpp"

using namespace std;
using namespace crocore;

void blank_background();
std::string find_mouse_handler();
std::string find_keyboard_handler();
std::string find_device_handler(const std::string &the_dev_name);
void get_input_file_descriptors(int *mouse_fd, int *kb_fd, int *touch_fd);
int32_t code_lookup(int32_t the_keycode);

namespace kinski
{

namespace
{
    gl::vec2 current_mouse_pos;
    uint32_t button_modifiers = 0, key_modifiers = 0;
    uint32_t current_touch_index = 0;
    Touch current_touches[10];
};

void read_keyboard(kinski::App* the_app, int the_file_descriptor);
void read_mouse_and_touch(kinski::App* the_app, int the_file_descriptor);

EGL_App::EGL_App(int argc, char *argv[]):
App(argc, argv),
m_context(new ESContext),
m_mouse_fd(0),
m_keyboard_fd(0),
m_touch_fd(0)
{

}

EGL_App::~EGL_App()
{
    eglMakeCurrent(m_context->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(m_context->eglDisplay, m_context->eglSurface);
    eglDestroyContext(m_context->eglDisplay, m_context->eglContext);
    eglTerminate(m_context->eglDisplay);

    m_timer_device_scan.cancel();

    if(m_mouse_fd){ close(m_mouse_fd); }
    if(m_keyboard_fd){ close(m_keyboard_fd); }
    if(m_touch_fd){ close(m_touch_fd); }
}

// internal initialization. performed when run is invoked
void EGL_App::init()
{
    gettimeofday(&m_startTime, NULL);

    esInitContext(m_context.get());

    esCreateWindow(m_context.get(), name().c_str(),
                   ES_WINDOW_RGB | ES_WINDOW_ALPHA | ES_WINDOW_DEPTH /*| ES_WINDOW_MULTISAMPLE*/);

    // init gl::Context object
    auto platform_data = std::make_shared<gl::PlatformDataEGL>(m_context->eglDisplay,
                                                               m_context->eglContext,
                                                               m_context->eglSurface);
    gl::create_context(platform_data);

    // set graphical log stream
    crocore::g_logger.add_outstream(&m_outstream_gl);

    // version
    LOG_INFO<<"OpenGL: " << glGetString(GL_VERSION);
    LOG_INFO<<"GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION);

    set_window_size(gl::vec2(m_context->width, m_context->height));
    LOG_INFO<<"Context: " << gl::window_dimension().x << " x " << gl::window_dimension().y;

    // file search paths
    if(!args().empty()){ fs::add_search_path(fs::get_directory_part(args().front())); }
    // fs::add_search_path("./");
    fs::add_search_path("./res");
    fs::add_search_path("/usr/local/share/fonts");

    get_input_file_descriptors(&m_mouse_fd, &m_keyboard_fd, &m_touch_fd);

    m_timer_device_scan = Timer(background_queue().io_service(), [this]()
    {
        int mouse_fd = m_mouse_fd, keyboard_fd = m_keyboard_fd;
        int *mp = nullptr, *kp = nullptr;
        bool has_changed = false;

        // check for keyboard and mouse being added/removed
        auto mouse_handler = find_mouse_handler();
        auto kb_handler = find_keyboard_handler();

        if((!m_keyboard_fd && !kb_handler.empty()) ||
           (m_keyboard_fd && kb_handler.empty()))
        {
            keyboard_fd = 0;
            kp = &keyboard_fd;
            has_changed = true;
        }
        if((!m_mouse_fd && !mouse_handler.empty()) ||
           (m_mouse_fd && mouse_handler.empty()))
        {
            mouse_fd = 0;
            mp = &mouse_fd;
            has_changed = true;
        }
        if(!has_changed){ return; }

        get_input_file_descriptors(mp, kp, nullptr);
        main_queue().post([this, mouse_fd, keyboard_fd]
        {
            if(mouse_fd && !m_mouse_fd)
            {
                LOG_TRACE << "mouse connected";
                m_mouse_fd = mouse_fd;
            }
            else if(!mouse_fd && m_mouse_fd)
            {
                 LOG_TRACE << "mouse disconnected";
                 close(m_mouse_fd); m_mouse_fd = 0;
            }
            if(keyboard_fd && !m_keyboard_fd)
            {
                LOG_TRACE << "keyboard connected";
                m_keyboard_fd = keyboard_fd;
            }
            else if(!keyboard_fd && m_keyboard_fd)
            {
                LOG_TRACE << "keyboard disconnected";
                close(m_keyboard_fd); m_keyboard_fd = 0;
            }
        });
    });
    m_timer_device_scan.set_periodic();
    m_timer_device_scan.expires_from_now(5.0);

    // make sure touchscreen backlight stays on
    // TODO: use timer here
    // set_lcd_backlight(true);

    // center cursor
    current_mouse_pos = gl::window_dimension() / 2.f;

    // init imgui
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
//    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    gui::init(this);

    // user setup hook
    setup();
}

void EGL_App::set_window_size(const glm::vec2 &size)
{
    App::set_window_size(size);
    gl::set_window_dimension(size);
    resize(size[0], size[1]);
}

void EGL_App::draw_internal()
{
    gl::reset_state();

    // fire user draw-callback
    draw();

    // draw tweakbar
    if(display_gui())
    {
        // console output
        outstream_gl().draw();

        // render and draw gui
        gui::render();
    }

    if(cursor_visible() && (m_mouse_fd || m_touch_fd))
    {
         gl::draw_points_2D({current_mouse_pos}, gl::COLOR_RED, 5.f);
    }
}

void EGL_App::post_draw()
{
    gui::end_frame();
}

void EGL_App::swap_buffers()
{
    esSwapBuffer(m_context.get());
}

double EGL_App::get_application_time()
{
    timeval now;
    gettimeofday(&now, NULL);
    return (double)(now.tv_sec - m_startTime.tv_sec + (now.tv_usec - m_startTime.tv_usec) * 1e-6);
}

void EGL_App::set_cursor_position(float x, float y)
{
    current_mouse_pos = glm::clamp(gl::vec2(x, y), gl::vec2(0),
                                   gl::window_dimension() - gl::vec2(1));
}

gl::vec2 EGL_App::cursor_position() const
{
    return current_mouse_pos;
}

const MouseEvent EGL_App::mouse_state() const
{
    MouseEvent e(0, current_mouse_pos.x, current_mouse_pos.y, button_modifiers | key_modifiers,
                 glm::ivec2(0.f));
    return e;
}

void EGL_App::teardown()
{
    gui::shutdown();
}

void EGL_App::set_lcd_backlight(bool b) const
{
    const auto bl_path = "/sys/class/backlight/rpi_backlight/bl_power";

    if(fs::exists(bl_path))
    {
        std::string cmd = string("sudo bash -c \"echo ") + (b ? "0" : "1") +
        " >  " + bl_path + "\"";
        crocore::syscall(cmd);
    }
    crocore::syscall("setterm --blank 0");//--powersave off --powerdown 0
}

void EGL_App::poll_events()
{
    read_mouse_and_touch(this, m_mouse_fd);
    read_mouse_and_touch(this, m_touch_fd);
    read_keyboard(this, m_keyboard_fd);

    gui::process_joystick_input(get_joystick_states());
    gui::new_frame();
}

void read_keyboard(kinski::App* the_app, int the_file_descriptor)
{
    struct input_event ev[64];
    int rd = 0;

    if(the_file_descriptor){ rd = read(the_file_descriptor, ev, sizeof(ev)); }

    if(rd > 0)
    {
        int count, n = 0;
        struct input_event *evp;
        count = rd / sizeof(struct input_event);
        while(count--)
        {
            evp = &ev[n++];
            if(evp->type == EV_KEY)
            {
                switch(evp->code)
                {
                    case KEY_LEFTCTRL:
                    case KEY_RIGHTCTRL:
                        if(evp->value == 1){ key_modifiers |= KeyEvent::CTRL_DOWN; }
                        else if(evp->value == 0){ key_modifiers ^= KeyEvent::CTRL_DOWN; }
                        break;

                    case KEY_LEFTSHIFT:
                    case KEY_RIGHTSHIFT:
                        if(evp->value == 1){ key_modifiers |= KeyEvent::SHIFT_DOWN; }
                        else if(evp->value == 0){ key_modifiers ^= KeyEvent::SHIFT_DOWN; }
                        break;

                    case KEY_LEFTALT:
                    case KEY_RIGHTALT:
                        if(evp->value == 1){ key_modifiers |= KeyEvent::ALT_DOWN; }
                        else if(evp->value == 0){ key_modifiers ^= KeyEvent::ALT_DOWN; }
                        break;

                    case KEY_LEFTMETA:
                    case KEY_RIGHTMETA:
                        if(evp->value == 1){ key_modifiers |= KeyEvent::META_DOWN; }
                        else if(evp->value == 0){ key_modifiers ^= KeyEvent::META_DOWN; }
                        break;

                    default:
                        break;
                }
                uint32_t key_code = code_lookup(evp->code);
                KeyEvent e(key_code, key_code, key_modifiers);

                if(evp->value == 0)
                {
                    gui::key_release(e);
                    if(!ImGui::GetIO().WantCaptureKeyboard){ the_app->key_release(e); }
                }
                else if(evp->value == 1 || evp->value == 2)
                {
                    gui::key_press(e);
                    gui::char_callback(e.code());
                    if(!ImGui::GetIO().WantCaptureKeyboard){ the_app->key_press(e); }
                }

                // right place here !?
                if(key_code == Key::_ESCAPE){ the_app->set_running(false); }
            }
        }
    }
}

void read_mouse_and_touch(kinski::App* the_app, int the_file_descriptor)
{
    struct input_event ev[64];
    int rd = 0;

    // Read events from mouse
    if(the_file_descriptor){ rd = read(the_file_descriptor, ev, sizeof(ev)); }

    if(rd > 0)
    {
        int count, n;
        struct input_event *evp;

        count = rd / sizeof(struct input_event);
        n = 0;

        while(count--)
        {
            evp = &ev[n++];
            // LOG_DEBUG << "type: " << evp->type << " -- code:" << evp->code;
            bool touch_id_changed = false, generate_event = true;

            // mouse press / release or touch
            if(evp->type == 1)
            {
                switch(evp->code)
                {
                    case BTN_LEFT:
                      if(evp->value){ button_modifiers |= MouseEvent::LEFT_DOWN; }
                      else{ button_modifiers ^= MouseEvent::LEFT_DOWN; }
                      break;

                    case BTN_MIDDLE:
                      if(evp->value){ button_modifiers |= MouseEvent::MIDDLE_DOWN; }
                      else{ button_modifiers ^= MouseEvent::MIDDLE_DOWN;}
                      break;

                    case BTN_RIGHT:
                      if(evp->value){ button_modifiers |= MouseEvent::RIGHT_DOWN; }
                      else{ button_modifiers ^= MouseEvent::RIGHT_DOWN;}
                      break;

                    case BTN_TOUCH:
                      if(evp->value){ button_modifiers |= MouseEvent::TOUCH_DOWN; }
                      else{ button_modifiers ^= MouseEvent::TOUCH_DOWN;}
                      break;

                    default:
                      break;
                }
            }

            // mouse move / wheel
            else if(evp->type == 2)
            {
                // Mouse Left/Right or Up/Down
                if(evp->code == REL_X){ current_mouse_pos.x += evp->value; }
                else if(evp->code == REL_Y){ current_mouse_pos.y += evp->value; }

                current_mouse_pos = glm::clamp(current_mouse_pos, gl::vec2(0),
                                               gl::window_dimension() - gl::vec2(1));

            }

            // touch event
            else if(evp->type == 3)
            {
                switch(evp->code)
                {
                    // MT slot being modified
                    case ABS_MT_SLOT:
                        // LOG_DEBUG << "ABS_MT_SLOT: " << evp->value;
                        current_touch_index = evp->value;
                        generate_event = false;
                        break;

                    // ABS_MT_TRACKING_ID
                    case ABS_MT_TRACKING_ID:
                        // LOG_DEBUG << "ABS_MT_TRACKING_ID: "  << evp->value;
                        current_touches[current_touch_index].m_slot_index = current_touch_index;
                        current_touches[current_touch_index].m_id = evp->value;
                        touch_id_changed = true;
                        break;

                    case ABS_X:
                    case ABS_Y:
                        break;

                    case ABS_MT_POSITION_X:
                        current_touches[current_touch_index].m_position.x = evp->value;
                        current_mouse_pos.x = current_touches[0].m_position.x;
                        break;

                    case ABS_MT_POSITION_Y:
                        current_touches[current_touch_index].m_position.y = evp->value;
                        current_mouse_pos.y = current_touches[0].m_position.y;
                        break;

                    default:
                        break;
                }

            }
            else
            {
                // type not handled yet
                // LOG_DEBUG << "unhandled event -- type: " << evp->type << " -- code:" << evp->code;
            }

            // skip event generation, continue loop
            if(!generate_event){ continue; }

            uint32_t bothMods = key_modifiers | button_modifiers;
            MouseEvent e(button_modifiers, current_mouse_pos.x,
                         current_mouse_pos.y, bothMods, glm::ivec2(0, evp->value), current_touch_index,
                         current_touches[current_touch_index].m_id);

            // press /release
            if(evp->type == 1)
            {
                if(evp->value){ gui::mouse_press(e); }

                if(!ImGui::GetIO().WantCaptureMouse)
                {
                    if(evp->value){ the_app->mouse_press(e); }
                    else{ the_app->mouse_release(e); }
                }
            }
            else if(evp->type == 2 || evp->type == 3)
            {
                if(evp->code == REL_WHEEL){ gui::mouse_wheel(e); }

                if(!ImGui::GetIO().WantCaptureMouse)
                {
                    if(evp->code == REL_WHEEL){ the_app->mouse_wheel(e); }
                    else if(button_modifiers){ the_app->mouse_drag(e); }
                    else{ the_app->mouse_move(e); }
                }
            }

            if(e.is_touch() || touch_id_changed)
            {
                // generate touch-set
                std::set<const Touch*> touches;

                // for(uint32_t i = 0; i < 10; i++)
                for(Touch &t : current_touches)
                {
                    if(t.m_id != -1){ touches.insert(&t); }
                }

                if(touch_id_changed)
                {
                    if(evp->value == -1){ the_app->touch_end(e, touches); }
                    else{ the_app->touch_begin(e, touches); }
                }
                else{ the_app->touch_move(e, touches); }
            }
        }
    }
}
}// namespace

std::string find_device_handler(const std::string &the_dev_name)
{
    // get list of input-devices
    string dev_str = crocore::fs::read_file("/proc/bus/input/devices");
    auto lines = crocore::split(dev_str, '\n');
    const std::string handler_token = "H:";
    bool found_dev_name = false;
    string evt_handler_name = "not_found";

    for(const auto &l : lines)
    {
        if(!found_dev_name && (l.find(the_dev_name) != std::string::npos))
        { found_dev_name = true; }

        if(found_dev_name)
        {
            if(l.find(handler_token) != std::string::npos)
            {
                auto splits = crocore::split(l, '=');

                if(!splits.empty()){ splits = crocore::split(splits.back(), ' '); }
                if(!splits.empty()){ evt_handler_name = splits.back(); break; }
            }
        }
    }
    return crocore::fs::join_paths("/dev/input/", evt_handler_name);
}

std::string find_mouse_handler()
{
    std::string dir = "/dev/input/by-id";
    if(!crocore::fs::exists(dir)){ return ""; }
    auto input_handles = crocore::fs::get_directory_entries(dir);

    for(const auto &p : input_handles)
    {
        if(p.find("event-mouse") != string::npos){ return p; }
    }
    return "";
}

std::string find_keyboard_handler()
{
    std::string dir = "/dev/input/by-id";
    if(!crocore::fs::exists(dir)){ return ""; }
    auto input_handles = crocore::fs::get_directory_entries(dir);

    for(const auto &p : input_handles)
    {
        if(p.find("event-kbd") != string::npos){ return p; }
    }
    return "";
}

void get_input_file_descriptors(int *mouse_fd, int *kb_fd, int *touch_fd)
{
    auto mouse_handler = find_mouse_handler();
    auto kb_handler = find_keyboard_handler();

    if(!mouse_handler.empty() && mouse_fd)
    {
        *mouse_fd = open(mouse_handler.c_str(), O_RDONLY | O_NONBLOCK);
        ioctl(*mouse_fd, EVIOCGRAB, 1);
    }

    if(!kb_handler.empty() && kb_fd)
    {
        *kb_fd = open(kb_handler.c_str(), O_RDONLY | O_NONBLOCK);
        ioctl(*kb_fd, EVIOCGRAB, 1);
    }

    // check for valid pointer
    if(touch_fd)
    {
        // find touch device name
        auto touch_dev_path = find_device_handler("FT5406");

        if(crocore::fs::exists(touch_dev_path))
        {
            int result = -1;
            (void) result;
            *touch_fd = open(touch_dev_path.c_str(), O_RDONLY | O_NONBLOCK);
            result = ioctl(*touch_fd, EVIOCGRAB, 1);
            // printf("%s\n", (result == 0) ? "SUCCESS" : "FAILURE");

            char name[256] = "Unknown";
            result = ioctl(*touch_fd, EVIOCGNAME(sizeof(name)), name);
            LOG_TRACE << "found input: " << name;
        }
    }
}

int32_t code_lookup(int32_t the_keycode)
{
    static int32_t lookup_table[1024];
    static int32_t *ptr = nullptr;
    if(!ptr)
    {
        memset(lookup_table, 0, sizeof(lookup_table));
        ptr = lookup_table;

        ptr[KEY_RESERVED] = 0;
        ptr[KEY_ESC] = kinski::Key::_ESCAPE;
        ptr[KEY_1] = kinski::Key::_1;
        ptr[KEY_2] = kinski::Key::_2;
        ptr[KEY_3] = kinski::Key::_3;
        ptr[KEY_4] = kinski::Key::_4;
        ptr[KEY_5] = kinski::Key::_5;
        ptr[KEY_6] = kinski::Key::_6;
        ptr[KEY_7] = kinski::Key::_7;
        ptr[KEY_8] = kinski::Key::_8;
        ptr[KEY_9] = kinski::Key::_9;
        ptr[KEY_0] = kinski::Key::_0;
        ptr[KEY_MINUS] = kinski::Key::_MINUS;
        ptr[KEY_EQUAL] = kinski::Key::_EQUAL;
        ptr[KEY_BACKSPACE] = kinski::Key::_BACKSPACE;
        ptr[KEY_TAB] = kinski::Key::_TAB;
        ptr[KEY_Q] = kinski::Key::_Q;
        ptr[KEY_W] = kinski::Key::_W;
        ptr[KEY_E] = kinski::Key::_E;
        ptr[KEY_R] = kinski::Key::_R;
        ptr[KEY_T] = kinski::Key::_T;
        ptr[KEY_Y] = kinski::Key::_Y;
        ptr[KEY_U] = kinski::Key::_U;
        ptr[KEY_I] = kinski::Key::_I;
        ptr[KEY_O] = kinski::Key::_O;
        ptr[KEY_P] = kinski::Key::_P;
        ptr[KEY_ENTER] = kinski::Key::_ENTER;
        ptr[KEY_LEFTCTRL] = kinski::Key::_LEFT_CONTROL;
        ptr[KEY_A] = kinski::Key::_A;
        ptr[KEY_S] = kinski::Key::_S;
        ptr[KEY_D] = kinski::Key::_D;
        ptr[KEY_F] = kinski::Key::_F;
        ptr[KEY_G] = kinski::Key::_G;
        ptr[KEY_H] = kinski::Key::_H;
        ptr[KEY_J] = kinski::Key::_J;
        ptr[KEY_K] = kinski::Key::_K;
        ptr[KEY_L] = kinski::Key::_L;
        ptr[KEY_SEMICOLON] = kinski::Key::_SEMICOLON;
        ptr[KEY_APOSTROPHE] = kinski::Key::_APOSTROPHE;
        ptr[KEY_GRAVE] = kinski::Key::_GRAVE_ACCENT;
        ptr[KEY_LEFTSHIFT] = kinski::Key::_LEFT_SHIFT;
        ptr[KEY_BACKSLASH] = kinski::Key::_BACKSLASH;
        ptr[KEY_Z] = kinski::Key::_Z;
        ptr[KEY_X] = kinski::Key::_X;
        ptr[KEY_C] = kinski::Key::_C;
        ptr[KEY_V] = kinski::Key::_V;
        ptr[KEY_B] = kinski::Key::_B;
        ptr[KEY_N] = kinski::Key::_N;
        ptr[KEY_M] = kinski::Key::_M;
        ptr[KEY_COMMA] = kinski::Key::_COMMA;
        ptr[KEY_SLASH] = kinski::Key::_SLASH;
        ptr[KEY_RIGHTSHIFT] = kinski::Key::_RIGHT_SHIFT;
        ptr[KEY_LEFTALT] = kinski::Key::_LEFT_ALT;
        ptr[KEY_SPACE] = kinski::Key::_SPACE;
        ptr[KEY_CAPSLOCK] = kinski::Key::_CAPS_LOCK;
        ptr[KEY_SCROLLLOCK] = kinski::Key::_SCROLL_LOCK;
        ptr[KEY_F1] = kinski::Key::_F1;
        ptr[KEY_F2] = kinski::Key::_F2;
        ptr[KEY_F3] = kinski::Key::_F3;
        ptr[KEY_F4] = kinski::Key::_F4;
        ptr[KEY_F5] = kinski::Key::_F5;
        ptr[KEY_F6] = kinski::Key::_F6;
        ptr[KEY_F7] = kinski::Key::_F7;
        ptr[KEY_F8] = kinski::Key::_F8;
        ptr[KEY_F9] = kinski::Key::_F9;
        ptr[KEY_F10] = kinski::Key::_F10;
        ptr[KEY_F11] = kinski::Key::_F11;
        ptr[KEY_F12] = kinski::Key::_F12;
        ptr[KEY_F13] = kinski::Key::_F13;
        ptr[KEY_F14] = kinski::Key::_F14;
        ptr[KEY_F15] = kinski::Key::_F15;
        ptr[KEY_F16] = kinski::Key::_F16;
        ptr[KEY_F17] = kinski::Key::_F17;
        ptr[KEY_F18] = kinski::Key::_F18;
        ptr[KEY_F19] = kinski::Key::_F19;
        ptr[KEY_F20] = kinski::Key::_F20;
        ptr[KEY_F21] = kinski::Key::_F21;
        ptr[KEY_F22] = kinski::Key::_F22;
        ptr[KEY_F23] = kinski::Key::_F23;
        ptr[KEY_F24] = kinski::Key::_F24;
        ptr[KEY_NUMLOCK] = kinski::Key::_NUM_LOCK;
        ptr[KEY_KP0] = kinski::Key::_KP_0;
        ptr[KEY_KP1] = kinski::Key::_KP_1;
        ptr[KEY_KP2] = kinski::Key::_KP_2;
        ptr[KEY_KP3] = kinski::Key::_KP_3;
        ptr[KEY_KP4] = kinski::Key::_KP_4;
        ptr[KEY_KP5] = kinski::Key::_KP_5;
        ptr[KEY_KP6] = kinski::Key::_KP_6;
        ptr[KEY_KP7] = kinski::Key::_KP_7;
        ptr[KEY_KP8] = kinski::Key::_KP_8;
        ptr[KEY_KP9] = kinski::Key::_KP_9;
        ptr[KEY_KPMINUS] = kinski::Key::_KP_SUBTRACT;
        ptr[KEY_KPPLUS] = kinski::Key::_KP_ADD;
        ptr[KEY_KPDOT] = kinski::Key::_KP_DECIMAL;
        ptr[KEY_KPASTERISK] = kinski::Key::_KP_MULTIPLY;
        ptr[KEY_KPSLASH] = kinski::Key::_KP_DIVIDE;
        ptr[KEY_RIGHTCTRL] = kinski::Key::_RIGHT_CONTROL;
        ptr[KEY_RIGHTALT] = kinski::Key::_RIGHT_ALT;
        ptr[KEY_HOME] = kinski::Key::_HOME;
        ptr[KEY_UP] = kinski::Key::_UP;
        ptr[KEY_PAGEUP] = kinski::Key::_PAGE_UP;
        ptr[KEY_LEFT] = kinski::Key::_LEFT;
        ptr[KEY_RIGHT] = kinski::Key::_RIGHT;
        ptr[KEY_END] = kinski::Key::_END;
        ptr[KEY_DOWN] = kinski::Key::_DOWN;
        ptr[KEY_PAGEDOWN] = kinski::Key::_PAGE_DOWN;
        ptr[KEY_INSERT] = kinski::Key::_INSERT;
        ptr[KEY_DELETE] = kinski::Key::_DELETE;
        ptr[KEY_KPEQUAL] = kinski::Key::_KP_EQUAL;
        ptr[KEY_LEFTMETA] = 0;
        ptr[KEY_RIGHTMETA] = 0;
        ptr[KEY_MENU] = kinski::Key::_MENU;
        ptr[KEY_SCROLLUP] = 0;
        ptr[KEY_SCROLLDOWN] = 0;
        ptr[KEY_KPLEFTPAREN] = 0;
        ptr[KEY_KPRIGHTPAREN] = 0;
        ptr[KEY_UNKNOWN] = -1;
    }
    int32_t ret = lookup_table[the_keycode];
    if(!ret){ ret = the_keycode; }
    return ret;
}
