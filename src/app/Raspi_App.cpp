#include "Raspi_App.h"
#include <sys/time.h>
#include <regex.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>

#include "esUtil.h"
#undef countof

#include "core/file_functions.hpp"

using namespace std;

void get_input_file_descriptors(int *mouse_fd, int *kb_fd, int *touch_fd);
void handle_input_events(kinski::App *the_app, const int mouse_fd,
                         const int kb_fd){};
int32_t code_lookup(int32_t the_keycode);

namespace kinski
{

    namespace
    {
        gl::vec2 current_mouse_pos;
        uint32_t button_modifiers = 0, key_modifiers = 0;
    };

    void read_keyboard(kinski::App* the_app, int the_file_descriptor);
    void read_mouse_and_touch(kinski::App* the_app, int the_file_descriptor);

    Raspi_App::Raspi_App(int argc, char *argv[]):
    App(argc, argv),
    m_context(new ESContext),
    m_mouse_fd(0),
    m_keyboard_fd(0),
    m_touch_fd(0)
    {

    }

    Raspi_App::~Raspi_App()
    {
        eglMakeCurrent(m_context->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
        eglDestroySurface(m_context->eglDisplay, m_context->eglSurface);
        eglDestroyContext(m_context->eglDisplay, m_context->eglContext );
        eglTerminate(m_context->eglDisplay);

        if(m_mouse_fd){ close(m_mouse_fd); }
        if(m_keyboard_fd){ close(m_keyboard_fd); }
        if(m_touch_fd){ close(m_touch_fd); }
    }

    // internal initialization. performed when run is invoked
    void Raspi_App::init()
    {
        gettimeofday(&m_startTime, NULL);

        esInitContext(m_context.get());
        esCreateWindow(m_context.get(), name().c_str(), getWidth(), getHeight(),
                       ES_WINDOW_RGB | ES_WINDOW_ALPHA | ES_WINDOW_DEPTH /*| ES_WINDOW_MULTISAMPLE*/);

        // set graphical log stream
        Logger::get()->add_outstream(&m_outstream_gl);

        // version
        LOG_INFO<<"OpenGL: " << glGetString(GL_VERSION);
        LOG_INFO<<"GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION);

        set_window_size(gl::vec2(m_context->width, m_context->height));
        LOG_INFO<<"Context: " << gl::window_dimension().x << " x " << gl::window_dimension().y;

        // file search paths
        if(!args().empty()){ kinski::add_search_path(get_directory_part(args().front())); }
        kinski::add_search_path("");
        kinski::add_search_path("./");
        kinski::add_search_path("./res/");

        get_input_file_descriptors(&m_mouse_fd, &m_keyboard_fd, &m_touch_fd);

        // center cursor
        current_mouse_pos = gl::window_dimension() / 2.f;

        // user setup hook
        setup();
    }

    void Raspi_App::set_window_size(const glm::vec2 &size)
    {
        App::set_window_size(size);
        gl::set_window_dimension(size);

        // if(running())
        resize(size[0], size[1]);
    }

    void Raspi_App::draw_internal()
    {
        // clear framebuffer
        gl::clear();

        // fire user draw-callback
        draw();

        // draw tweakbar
        if(displayTweakBar())
        {
            // console output
            outstream_gl().draw();
        }
    }
    void Raspi_App::swapBuffers()
    {
        eglSwapBuffers(m_context->eglDisplay, m_context->eglSurface);
    }

    double Raspi_App::getApplicationTime()
    {
        timeval now;
        gettimeofday(&now, NULL);

        return (double)(now.tv_sec - m_startTime.tv_sec + (now.tv_usec - m_startTime.tv_usec) * 1e-6);
    }

    void Raspi_App::set_cursor_position(float x, float y)
    {
        current_mouse_pos = glm::clamp(gl::vec2(x, y), gl::vec2(0),
                                       gl::window_dimension() - gl::vec2(1));
    }

    gl::vec2 Raspi_App::cursor_position() const
    {
        return current_mouse_pos;
    }

    void Raspi_App::pollEvents()
    {
        read_mouse_and_touch(this, m_mouse_fd);
        read_mouse_and_touch(this, m_touch_fd);
        read_keyboard(this, m_keyboard_fd);
    }// pollEvents

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

                    if(evp->value == 0){ the_app->keyRelease(e); }
                    else if(evp->value == 1){ the_app->keyPress(e); }
                    else if(evp->value == 2){ the_app->keyPress(e); }

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
            int count,n;
            struct input_event *evp;

            count = rd / sizeof(struct input_event);
            n = 0;

            while(count--)
            {
                evp = &ev[n++];

                // LOG_DEBUG << "type: " << evp->type << "-- code:" << evp->code;

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
                          LOG_DEBUG << "TOUCH " << (evp->value ? "press" : "release");
                          break;

                        default:
                          break;
                    }
                    uint32_t bothMods = key_modifiers | button_modifiers;

                    MouseEvent e(button_modifiers, current_mouse_pos.x,
                                 current_mouse_pos.y, bothMods, glm::ivec2(0));
                    if(evp->value){ the_app->mousePress(e); }
                    else{ the_app->mouseRelease(e); }
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
                            break;

                        // ABS_MT_TRACKING_ID
                        case ABS_MT_TRACKING_ID:
                            LOG_DEBUG << "ABS_MT_TRACKING_ID: "  << evp->value;
                            break;

                        case ABS_X:
                            // LOG_DEBUG << "x: "  << evp->value;
                            // current_mouse_pos.x = evp->value;
                            break;

                        case ABS_Y:
                            // LOG_DEBUG << "y: "  << evp->value;
                            // current_mouse_pos.y = evp->value;
                            break;

                        case ABS_MT_POSITION_X:
                            // LOG_DEBUG << "x: "  << evp->value;
                            current_mouse_pos.x = evp->value;
                            break;

                        case ABS_MT_POSITION_Y:
                            // LOG_DEBUG << "y: "  << evp->value;
                            current_mouse_pos.y = evp->value;
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

                // generate mouse event
                if(evp->type == 2 || evp->type == 3)
                {
                    uint32_t bothMods = key_modifiers | button_modifiers;
                    MouseEvent e(button_modifiers, current_mouse_pos.x,
                    current_mouse_pos.y, bothMods, glm::ivec2(0, evp->value));

                    if(evp->code == REL_WHEEL){ the_app->mouseWheel(e); }
                    else if(button_modifiers){ the_app->mouseDrag(e); }
                    else{ the_app->mouseMove(e);}
                }
            }
        }
    }
}// namespace



void get_input_file_descriptors(int *mouse_fd, int *kb_fd, int *touch_fd)
{
    int keyboardFd = -1, mouseFd = -1, touchFd = -1;

    // init inputs
    DIR *dirp;
    struct dirent *dp;
    regex_t kbd, mouse;

    char fullPath[1024];
    const char *dirName = "/dev/input/by-id";

    if(regcomp(&kbd, "event-kbd", 0) != 0)
    {
        LOG_ERROR << "regcomp for kbd failed";
    }
    if(regcomp(&mouse, "event-mouse", 0) != 0)
    {
        LOG_ERROR << "regcomp for mouse failed";
    }
    if(!(dirp = opendir(dirName)))
    {
        LOG_ERROR << "couldn't open '/dev/input/by-id'";
    }

    int result = -1;
    (void)result;

    // Find any files that match the regex for keyboard or mouse
    do
    {
        errno = 0;
        if ((dp = readdir(dirp)) != NULL)
        {
            // printf("readdir (%s)\n", dp->d_name);
            if(regexec (&kbd, dp->d_name, 0, NULL, 0) == 0)
            {
                sprintf(fullPath,"%s/%s", dirName, dp->d_name);
                keyboardFd = open(fullPath, O_RDONLY | O_NONBLOCK);
                result = ioctl(keyboardFd, EVIOCGRAB, 1);
            }
            if(regexec (&mouse, dp->d_name, 0, NULL, 0) == 0)
            {
                sprintf(fullPath,"%s/%s", dirName, dp->d_name);
                mouseFd = open(fullPath, O_RDONLY | O_NONBLOCK);
                result = ioctl(mouseFd, EVIOCGRAB, 1);

                char name[256] = "Unknown";
                result = ioctl(mouseFd, EVIOCGNAME(sizeof(name)), name);
                LOG_INFO_IF(!result) << "found input: " << name;
            }
        }
    } while (dp != NULL);

    closedir(dirp);
    regfree(&kbd);
    regfree(&mouse);

    string touch_dev_path = "/dev/input/event2";

    if(kinski::file_exists(touch_dev_path))
    {
        // printf("match for the mouse = %s\n", dp->d_name);
        // sprintf(fullPath,"%s/%s", dirName, dp->d_name);
        sprintf(fullPath,touch_dev_path.c_str());
        touchFd = open(fullPath, O_RDONLY | O_NONBLOCK);

        // printf("%s Fd = %d\n", fullPath, mouseFd);
        // printf("Getting exclusive access: ");
        result = ioctl(touchFd, EVIOCGRAB, 1);
        printf("%s\n", (result == 0) ? "SUCCESS" : "FAILURE");

        char name[256] = "Unknown";
        result = ioctl(touchFd, EVIOCGNAME(sizeof(name)), name);
        LOG_INFO_IF(!result) << "found input: " << name;
    }
    
    if(keyboardFd != -1)
    {
        *kb_fd = keyboardFd;
        LOG_INFO << "keyboard detected";
    }

    if(mouseFd != -1)
    {
       *mouse_fd = mouseFd;
       LOG_INFO << "mouse detected";
    }

    if(touchFd != -1)
    {
       *touch_fd = touchFd;
       LOG_INFO << "touch-input detected";
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
