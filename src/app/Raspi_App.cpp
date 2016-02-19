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

namespace kinski
{
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

        esInitContext ( m_context.get() );
        esCreateWindow ( m_context.get(), name().c_str(), getWidth(), getHeight(),
                        ES_WINDOW_RGB | ES_WINDOW_ALPHA | ES_WINDOW_DEPTH  /*| ES_WINDOW_MULTISAMPLE*/);

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
                if(evp->type == 1)
                {
                    if(evp->code == BTN_LEFT)
                    {
                        if(evp->value == 1)   // Press
                        {
                            printf("Left button pressed\n");
                        }
                        else
                        {
                            printf("Left button released\n");
                        }
                    }
                }

                if(evp->type == 2)
                {
                    if(evp->code == 0)
                    {
                        // Mouse Left/Right
                        printf("Mouse moved left/right %d\n",evp->value);
                    }

                    if(evp->code == 1)
                    {
                        // Mouse Up/Down
                        printf("Mouse moved up/down %d\n",evp->value);
                    }
                }
            }
        }

    //   // Read events from keyboard
    //
    //   rd = read(keyboardFd,ev,sizeof(ev));
    //   if(rd > 0)
    //   {
    // int count,n;
    // struct input_event *evp;
    //
    // count = rd / sizeof(struct input_event);
    // n = 0;
    // while(count--)
    // {
    //     evp = &ev[n++];
    //     if(evp->type == 1)
    //     {
    //   if(evp->value == 1)
    //   {
    //       if(evp->code == KEY_LEFTCTRL)
    //
    //       {
    //     printf("Left Control key pressed\n");
    //
    //       }
    //       if(evp->code == KEY_LEFTMETA )
    //       {
    //     printf("Left Meta key pressed\n");
    //
    //       }
    //       if(evp->code == KEY_LEFTSHIFT)
    //       {
    //     printf("Left Shift key pressed\n");
    //
    //       }
    //   }
    //
    //
    //   if((evp->code == KEY_Q) && (evp->value == 1))
    //       ret = true;;
    //
    //
    //     }
    // }
    //
    //   }
    }
}

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

    if((keyboardFd == -1) || (mouseFd == -1))
    {
        LOG_ERROR << "couldn't open input file descriptors";
    }
    else
    {
        *mouse_fd = mouseFd;
        *kb_fd = keyboardFd;
    }
}
