#include "Raspi_App.h"
#include <sys/time.h>
#include <regex.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>

#include "esUtil.h"
#undef countof

#include "core/file_functions.h"

using namespace std;

void get_input_file_descriptors(int *mouse_fd, int *kb_fd);
void handle_input_events(kinski::App *the_app, const int mouse_fd,
                         const int kb_fd){};
int32_t code_lookup(uint32_t the_keycode);

namespace kinski
{

    namespace
    {
        gl::vec2 current_mouse_pos;
        uint32_t button_modifiers = 0, key_modifiers = 0;
    };

    Raspi_App::Raspi_App(const int width, const int height):
    App(width, height),
    m_context(new ESContext),
    m_mouse_fd(0),
    m_keyboard_fd(0)
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

        set_window_size(windowSize());

        // file search paths
        kinski::add_search_path("");
        kinski::add_search_path("./");
        kinski::add_search_path("./res/");

        get_input_file_descriptors(&m_mouse_fd, &m_keyboard_fd);

        // user setup hook
        setup();
    }

    void Raspi_App::set_window_size(const glm::vec2 &size)
    {
        App::set_window_size(size);
        gl::setWindowDimension(size);
        if(running()) resize(size[0], size[1]);
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

    void Raspi_App::pollEvents()
    {
        handle_input_events(this, m_mouse_fd, m_keyboard_fd);

        struct input_event ev[64];
        int rd;

        // Read events from mouse
        rd = read(m_mouse_fd, ev, sizeof(ev));

        if(rd > 0)
        {
            int count,n;
            struct input_event *evp;

            count = rd / sizeof(struct input_event);
            n = 0;

            while(count--)
            {
                evp = &ev[n++];

                // mouse press / release
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

                        default:
                          break;
                    }
                    uint32_t bothMods = key_modifiers | button_modifiers;

                    MouseEvent e(button_modifiers, current_mouse_pos.x,
                                 current_mouse_pos.y, bothMods, glm::ivec2(0));
                    if(evp->value){ mousePress(e); }
                    else{ mouseRelease(e); }
                }

                // mouse move / wheel
                else if(evp->type == 2)
                {
                    // Mouse Left/Right or Up/Down
                    if(evp->code == REL_X){ current_mouse_pos.x += evp->value; }
                    else if(evp->code == REL_Y){ current_mouse_pos.y += evp->value; }

                    current_mouse_pos = glm::clamp(current_mouse_pos, gl::vec2(0),
                                                   gl::windowDimension() - gl::vec2(1));

                    uint32_t bothMods = key_modifiers | button_modifiers;
                    MouseEvent e(button_modifiers, current_mouse_pos.x,
                                 current_mouse_pos.y, bothMods, glm::ivec2(0, evp->value));

                    if(evp->code == REL_WHEEL){ mouseWheel(e); }
                    else if(button_modifiers){ mouseDrag(e); }
                    else{ mouseMove(e);}
                }
            }
        }

        // Read events from keyboard
        rd = read(m_keyboard_fd, ev, sizeof(ev));

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

                    if(evp->value == 0){ keyRelease(e); }
                    else if(evp->value == 1){ keyPress(e); }
                    else if(evp->value == 2){ keyPress(e); }
                }
            }
        }

    }// pollEvents

}// namespace

void get_input_file_descriptors(int *mouse_fd, int *kb_fd)
{
    int keyboardFd = -1, mouseFd = -1;

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
                // printf("match for the kbd = %s\n", dp->d_name);
                sprintf(fullPath,"%s/%s", dirName, dp->d_name);
                keyboardFd = open(fullPath, O_RDONLY | O_NONBLOCK);
                // printf("%s Fd = %d\n",fullPath,keyboardFd);
                // printf("Getting exclusive access: ");
                result = ioctl(keyboardFd, EVIOCGRAB, 1);
                // printf("%s\n", (result == 0) ? "SUCCESS" : "FAILURE");

            }
            if(regexec (&mouse, dp->d_name, 0, NULL, 0) == 0)
            {
                // printf("match for the mouse = %s\n", dp->d_name);
                sprintf(fullPath,"%s/%s", dirName, dp->d_name);
                mouseFd = open(fullPath, O_RDONLY | O_NONBLOCK);

                // printf("%s Fd = %d\n", fullPath, mouseFd);
                // printf("Getting exclusive access: ");
                result = ioctl(mouseFd, EVIOCGRAB, 1);
                // printf("%s\n", (result == 0) ? "SUCCESS" : "FAILURE");
            }
        }
    } while (dp != NULL);

    closedir(dirp);

    regfree(&kbd);
    regfree(&mouse);

    if(keyboardFd != -1){ *kb_fd = keyboardFd; }
    else{ LOG_WARNING << "couldn't open a keyboard file-descriptor"; }

    if(mouseFd != -1){ *mouse_fd = mouseFd; }
    else{ LOG_WARNING << "couldn't open a mouse file-descriptor"; }
}

int32_t code_lookup(uint32_t the_keycode)
{
    static int32_t lookup_table[1024];
    static int32_t *ptr = nullptr;
    if(!ptr)
    {
      memset(lookup_table, 0, sizeof(lookup_table));
      ptr = lookup_table;

      ptr[KEY_RESERVED] = 0;
      ptr[KEY_ESC] = (int32_t)(kinski::KEY_ESCAPE);
      // ptr[KEY_1] = (int32_t)(kinski::KEY_1);
      // ptr[KEY_2] = (int32_t)(kinski::KEY_2);
      // ptr[KEY_3] = (int32_t)(kinski::KEY_3);
      // ptr[KEY_4] = (int32_t)(kinski::KEY_4);
      // ptr[KEY_5] = (int32_t)(kinski::KEY_5);
      // ptr[KEY_6] = (int32_t)(kinski::KEY_6);
      // ptr[KEY_7] = (int32_t)(kinski::KEY_7);
      // ptr[KEY_8] = (int32_t)(kinski::KEY_8);
      // ptr[KEY_9] = (int32_t)(kinski::KEY_9);
      // ptr[KEY_0] = (int32_t)(kinski::KEY_0);
      // ptr[KEY_MINUS] = (int32_t)(kinski::KEY_MINUS);
      // ptr[KEY_EQUAL] = (int32_t)(kinski::KEY_EQUAL);
      // ptr[KEY_BACKSPACE] = (int32_t)(kinski::KEY_BACKSPACE);
      // ptr[KEY_TAB] = (int32_t)(kinski::KEY_TAB);
      // ptr[KEY_Q] = (int32_t)(kinski::KEY_Q);
      // ptr[KEY_W] = (int32_t)(kinski::KEY_W);
      // ptr[KEY_E] = (int32_t)(kinski::KEY_E);
      // ptr[KEY_R] = (int32_t)(kinski::KEY_R);
      // ptr[KEY_T] = (int32_t)(kinski::KEY_T);
      // ptr[KEY_Y] = (int32_t)(kinski::KEY_Y);
      // ptr[KEY_U] = (int32_t)(kinski::KEY_U);
      // ptr[KEY_I] = (int32_t)(kinski::KEY_I);
      // ptr[KEY_O] = (int32_t)(kinski::KEY_O);
      // ptr[KEY_P] = (int32_t)(kinski::KEY_P);
      // ptr[KEY_ENTER] = (int32_t)(kinski::KEY_ENTER);
      // ptr[KEY_LEFTCTRL] = (int32_t)(kinski::KEY_LEFT_CONTROL);
      // ptr[KEY_A] = (int32_t)(kinski::KEY_A);
      // ptr[KEY_S] = (int32_t)(kinski::KEY_S);
      // ptr[KEY_D] = (int32_t)(kinski::KEY_D);
      // ptr[KEY_F] = (int32_t)(kinski::KEY_F);
      // ptr[KEY_G] = (int32_t)(kinski::KEY_G);
      // ptr[KEY_H] = (int32_t)(kinski::KEY_H);
      // ptr[KEY_J] = (int32_t)(kinski::KEY_J);
      // ptr[KEY_K] = (int32_t)(kinski::KEY_K);
      // ptr[KEY_L] = (int32_t)(kinski::KEY_L);
      // ptr[KEY_SEMICOLON] = (int32_t)(kinski::KEY_SEMICOLON);
      // ptr[KEY_APOSTROPHE] = (int32_t)(kinski::KEY_APOSTROPHE);
      // ptr[KEY_GRAVE] = (int32_t)(kinski::KEY_GRAVE_ACCENT);
      // ptr[KEY_LEFTSHIFT] = (int32_t)(kinski::KEY_LEFT_SHIFT);
      // ptr[KEY_BACKSLASH] = (int32_t)(kinski::KEY_BACKSLASH);
      // ptr[KEY_Z] = (int32_t)(kinski::KEY_Z);
      // ptr[KEY_X] = (int32_t)(kinski::KEY_X);
      // ptr[KEY_C] = (int32_t)(kinski::KEY_C);
      // ptr[KEY_V] = (int32_t)(kinski::KEY_V);
      // ptr[KEY_B] = (int32_t)(kinski::KEY_B);
      // ptr[KEY_N] = (int32_t)(kinski::KEY_N);
      // ptr[KEY_M] = (int32_t)(kinski::KEY_M);
      // ptr[KEY_COMMA] = (int32_t)(kinski::KEY_COMMA);
      // ptr[KEY_SLASH] = (int32_t)(kinski::KEY_SLASH);
      // ptr[KEY_RIGHTSHIFT] = (int32_t)(kinski::KEY_RIGHT_SHIFT);
      // ptr[KEY_KPASTERISK] = (int32_t)(kinski::KEY_L);
      // ptr[KEY_LEFTALT] = (int32_t)(kinski::KEY_LEFT_ALT);
      // ptr[KEY_SPACE] = (int32_t)(kinski::KEY_SPACE);
      // ptr[KEY_CAPSLOCK] = (int32_t)(kinski::KEY_CAPS_LOCK);
      // ptr[KEY_SCROLLLOCK] = (int32_t)(kinski::KEY_SCROLL_LOCK);
      // ptr[KEY_F1] = (int32_t)(kinski::KEY_F1);
      // ptr[KEY_F2] = (int32_t)(kinski::KEY_F2);
      // ptr[KEY_F3] = (int32_t)(kinski::KEY_F3);
      // ptr[KEY_F4] = (int32_t)(kinski::KEY_F4);
      // ptr[KEY_F5] = (int32_t)(kinski::KEY_F5);
      // ptr[KEY_F6] = (int32_t)(kinski::KEY_F6);
      // ptr[KEY_F7] = (int32_t)(kinski::KEY_F7);
      // ptr[KEY_F8] = (int32_t)(kinski::KEY_F8);
      // ptr[KEY_F9] = (int32_t)(kinski::KEY_F9);
      // ptr[KEY_F10] = (int32_t)(kinski::KEY_F10);
      // ptr[KEY_F11] = (int32_t)(kinski::KEY_F11);
      // ptr[KEY_F12] = (int32_t)(kinski::KEY_F12);
      // ptr[KEY_F13] = (int32_t)(kinski::KEY_F13);
      // ptr[KEY_F14] = (int32_t)(kinski::KEY_F14);
      // ptr[KEY_F15] = (int32_t)(kinski::KEY_F15);
      // ptr[KEY_F16] = (int32_t)(kinski::KEY_F16);
      // ptr[KEY_F17] = (int32_t)(kinski::KEY_F17);
      // ptr[KEY_F18] = (int32_t)(kinski::KEY_F18);
      // ptr[KEY_F19] = (int32_t)(kinski::KEY_F19);
      // ptr[KEY_F20] = (int32_t)(kinski::KEY_F20);
      // ptr[KEY_F21] = (int32_t)(kinski::KEY_F21);
      // ptr[KEY_F22] = (int32_t)(kinski::KEY_F22);
      // ptr[KEY_F23] = (int32_t)(kinski::KEY_F23);
      // ptr[KEY_F24] = (int32_t)(kinski::KEY_F24);
      // ptr[KEY_NUMLOCK] = (int32_t)(kinski::KEY_NUM_LOCK);
      // ptr[KEY_KP0] = (int32_t)(kinski::KEY_KP_0);
      // ptr[KEY_KP1] = (int32_t)(kinski::KEY_KP_1);
      // ptr[KEY_KP2] = (int32_t)(kinski::KEY_KP_2);
      // ptr[KEY_KP3] = (int32_t)(kinski::KEY_KP_3);
      // ptr[KEY_KP4] = (int32_t)(kinski::KEY_KP_4);
      // ptr[KEY_KP5] = (int32_t)(kinski::KEY_KP_5);
      // ptr[KEY_KP6] = (int32_t)(kinski::KEY_KP_6);
      // ptr[KEY_KP7] = (int32_t)(kinski::KEY_KP_7);
      // ptr[KEY_KP8] = (int32_t)(kinski::KEY_KP_8);
      // ptr[KEY_KP9] = (int32_t)(kinski::KEY_KP_9);
      // ptr[KEY_KPMINUS] = (int32_t)(kinski::KEY_KP_SUBTRACT);
      // ptr[KEY_KPPLUS] = (int32_t)(kinski::KEY_KP_ADD);
      // ptr[KEY_KPDOT] = (int32_t)(kinski::KEY_KP_DECIMAL);
      // ptr[KEY_RIGHTCTRL] = (int32_t)(kinski::KEY_RIGHT_CONTROL);
      // ptr[KEY_KPSLASH] = 0;
      // ptr[KEY_RIGHTALT] = (int32_t)(kinski::KEY_RIGHT_ALT);
      // ptr[KEY_HOME] = (int32_t)(kinski::KEY_HOME);
      // ptr[KEY_UP] = (int32_t)(kinski::KEY_UP);
      // ptr[KEY_PAGEUP] = (int32_t)(kinski::KEY_PAGE_UP);
      // ptr[KEY_LEFT] = (int32_t)(kinski::KEY_LEFT);
      // ptr[KEY_RIGHT] = (int32_t)(kinski::KEY_RIGHT);
      // ptr[KEY_END	] = (int32_t)(kinski::KEY_END);
      // ptr[KEY_DOWN] = (int32_t)(kinski::KEY_DOWN);
      // ptr[KEY_PAGEDOWN] = (int32_t)(kinski::KEY_PAGE_DOWN);
      // ptr[KEY_INSERT] = (int32_t)(kinski::KEY_INSERT);
      // ptr[KEY_DELETE] = (int32_t)(kinski::KEY_DELETE);
      // ptr[KEY_KPEQUAL] = (int32_t)(kinski::KEY_KP_EQUAL);
      // ptr[KEY_LEFTMETA] = 0;
      // ptr[KEY_RIGHTMETA] = 0;
      // ptr[KEY_MENU] = (int32_t)(kinski::KEY_MENU);
      // ptr[KEY_SCROLLUP] = 0;
      // ptr[KEY_SCROLLDOWN] = 0;
      // ptr[KEY_KPLEFTPAREN] = 0;
      // ptr[KEY_KPRIGHTPAREN] = 0;
      ptr[KEY_UNKNOWN] = -1;
    }
    int32_t ret = lookup_table[the_keycode];
    if(!ret){ ret = the_keycode; }
    return ret;
}
