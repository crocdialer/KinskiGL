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

uint32_t lookup_table[512];
void get_input_file_descriptors(int *mouse_fd, int *kb_fd);
void handle_input_events(kinski::App *the_app, const int mouse_fd,
                         const int kb_fd){};
uint32_t code_lookup(uint32_t the_keycode);

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

uint32_t code_lookup(uint32_t the_keycode)
{
    static uint32_t *ptr = nullptr;
    if(!ptr)
    {
      memset(lookup_table, 0, sizeof(lookup_table));
      ptr = lookup_table;
      // ptr[]
    }
    uint32_t ret = lookup_table[the_keycode];
    if(!ret){ ret = the_keycode; }
    return ret;
}
