    // __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "gl/gl.hpp"
#include "gl/Material.hpp"
#include "GLFW_App.hpp"
#include "imgui/imgui_impl_glfw_gl3.h"

#if defined(KINSKI_LINUX)
#include <GL/glx.h>
#elif defined (__APPLE__) || defined(MACOSX)
#include <OpenGL/CGLCurrent.h>
#endif

using namespace std;

namespace kinski
{

/////////////////////////////////////////////////////////////////

    ImVec4 im_color_cast(const gl::Color &the_color)
    {
        return *reinterpret_cast<const ImVec4*>(&the_color);
    }

    inline ImVec4 im_color_cast(const gl::vec3 &the_color)
    {
        return im_color_cast(gl::Color(the_color, 1.f));
    }

/////////////////////////////////////////////////////////////////

    GLFW_WindowPtr GLFW_Window::create(int width, int height, const std::string &theName,
                                       bool fullscreen, int monitor_index, GLFWwindow* share)
    {
        return GLFW_WindowPtr(new GLFW_Window(width, height, theName, fullscreen,
                                              monitor_index, share));
    }

    GLFW_WindowPtr GLFW_Window::create(int width, int height, const std::string &theName)
    {
        return GLFW_WindowPtr(new GLFW_Window(width, height, theName));
    }

    GLFW_Window::GLFW_Window(int width, int height, const std::string &theName, bool fullscreen,
                             int monitor_index, GLFWwindow* share)
    {
         int monitor_count = 0;
         GLFWmonitor **monitors = glfwGetMonitors(&monitor_count);
         monitor_index = clamp(monitor_index, 0, monitor_count - 1);

         m_handle = glfwCreateWindow(width, height,
                                     theName.c_str(),
                                     fullscreen ? monitors[monitor_index] : NULL,
                                     share);
        if(!m_handle) throw CreateWindowException();
        glfwMakeContextCurrent(m_handle);
    }

    GLFW_Window::GLFW_Window(int width, int height, const std::string &theName)
    {
        m_handle = glfwCreateWindow(width, height, theName.c_str(), NULL, NULL);
        if(!m_handle) throw CreateWindowException();
        glfwMakeContextCurrent(m_handle);
    }

    gl::ivec2 GLFW_Window::size() const
    {
        int w, h;
        glfwGetWindowSize(m_handle, &w, &h);
        return gl::ivec2(w, h);
    }

    void GLFW_Window::set_size(const gl::ivec2 &the_sz)
    {
        glfwSetWindowSize(m_handle, the_sz.x, the_sz.y);
    }

    gl::ivec2 GLFW_Window::position() const
    {
        int x, y;
        glfwGetWindowPos(m_handle, &x, &y);
        return gl::vec2(x, y);
    }

    void GLFW_Window::set_position(const gl::ivec2 &the_pos)
    {
        glfwSetWindowPos(m_handle, the_pos.x, the_pos.y);
    }

    std::string GLFW_Window::title(const std::string &the_name) const
    {
        return m_title;
    }

    void GLFW_Window::set_title(const std::string &the_title)
    {
        m_title = the_title;
        glfwSetWindowTitle(m_handle, the_title.c_str());
    }

    GLFW_Window::~GLFW_Window()
    {
        glfwDestroyWindow(m_handle);
    }

    gl::ivec2 GLFW_Window::framebuffer_size() const
    {
        int w, h;
        glfwGetFramebufferSize(m_handle, &w, &h);
        return gl::vec2(w, h);
    }

    void GLFW_Window::draw() const
    {
        glfwMakeContextCurrent(m_handle);
        gl::context()->set_current_context_id(m_handle);
        gl::set_window_dimension(framebuffer_size());
        if(m_draw_function){ m_draw_function(); }
    }

    uint32_t GLFW_Window::monitor_index() const
    {
        uint32_t ret = 0;
        int nmonitors, i;
        int wx, wy, ww, wh;
        int mx, my, mw, mh;
        int overlap, bestoverlap;
        GLFWmonitor **monitors;
        const GLFWvidmode *mode;
        bestoverlap = 0;

        glfwGetWindowPos(m_handle, &wx, &wy);
        glfwGetWindowSize(m_handle, &ww, &wh);
        monitors = glfwGetMonitors(&nmonitors);

        for (i = 0; i < nmonitors; i++)
        {
            mode = glfwGetVideoMode(monitors[i]);
            glfwGetMonitorPos(monitors[i], &mx, &my);
            mw = mode->width;
            mh = mode->height;

            overlap =
            max(0, min(wx + ww, mx + mw) - max(wx, mx)) *
            max(0, min(wy + wh, my + mh) - max(wy, my));

            if(bestoverlap < overlap)
            {
                bestoverlap = overlap;
                ret = i;
            }
        }
        return ret;
    }

    GLFW_App::GLFW_App(int argc, char *argv[]):
    App(argc, argv),
    m_lastWheelPos(0)
    {
        glfwSetErrorCallback(&s_error_cb);
    }

    GLFW_App::~GLFW_App()
    {
        // close all windows
        m_windows.clear();

        // terminate GLFW
        glfwTerminate();
    }

    void GLFW_App::init()
    {
        // Initialize GLFW
        if( !glfwInit() )
        {
            throw Exception("GLFW failed to initialize");
        }

        int num_color_bits = 8;

        glfwWindowHint(GLFW_RED_BITS, num_color_bits);
        glfwWindowHint(GLFW_GREEN_BITS, num_color_bits);
        glfwWindowHint(GLFW_BLUE_BITS, num_color_bits);

        // request an OpenGl 4.1 Context
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        #if defined(KINSKI_MAC)
          glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        #else
          glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        #endif
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 4);

        // create the window
        gl::vec2 default_sz(1280, 720);
        auto main_window = GLFW_Window::create(default_sz.x, default_sz.y, name(), fullscreen());
        add_window(main_window);

        // create gl::Context object
        std::shared_ptr<gl::PlatformData> pd;
#if defined(KINSKI_MAC)
        pd = std::make_shared<gl::PlatformDataCGL>(CGLGetCurrentContext());
#endif
        gl::create_context(pd);
        gl::context()->set_current_context_id(main_window->handle());

        // set graphical log stream
        Logger::get()->add_outstream(&m_outstream_gl);

        // version
        LOG_INFO<<"OpenGL: " << glGetString(GL_VERSION);
        LOG_INFO<<"GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION);

        glfwSetMonitorCallback(&GLFW_App::s_monitor_func);
        glfwSwapInterval(1);
        glClearColor(0, 0, 0, 1);

        // file search paths
        if(!args().empty()){ fs::add_search_path(fs::get_directory_part(args().front())); }
        fs::add_search_path("./", true);
        fs::add_search_path("./res", true);
        fs::add_search_path("../Resources", true);

        //---------------------------------
#if defined(KINSKI_MAC)
        fs::add_search_path("/Library/Fonts");
        fs::add_search_path("~/Library/Fonts");
#elif defined(KINSKI_LINUX)
        fs::add_search_path("~/.local/share/fonts");
        fs::add_search_path("/usr/local/share/fonts");
#endif
        //---------------------------------

        set_window_size(main_window->size());

        // ImGui
        ImGui::CreateContext();
//        ImGuiIO& io = ImGui::GetIO();
//        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
//        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
        ImGui_ImplGlfwGL3_Init(main_window->handle(), false, "#version 410");
        ImGuiStyle& im_style = ImGui::GetStyle();
        im_style.Colors[ImGuiCol_TitleBgActive] = im_color_cast(gl::COLOR_ORANGE.rgb * 0.5f);
        im_style.Colors[ImGuiCol_FrameBg] = im_color_cast(gl::COLOR_WHITE.rgb * 0.07f);
        im_style.Colors[ImGuiCol_FrameBgHovered] = im_style.Colors[ImGuiCol_FrameBgActive] =
                im_color_cast(gl::COLOR_ORANGE.rgb * 0.5f);

#if GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 2
        glfwSetJoystickCallback(&GLFW_App::s_joystick_cb);
#endif

        // call user defined setup callback
        setup();
    }

    void GLFW_App::swap_buffers()
    {
        for(const auto &window : m_windows)
        {
            glfwSwapBuffers(window->handle());
        }
    }

    void GLFW_App::set_window_size(const glm::vec2 &size)
    {
        App::set_window_size(size);
        resize(size.x, size.y);

        if(!m_windows.empty())
        {
            m_windows.front()->set_size(size);
            
            auto fb_size = m_windows.front()->framebuffer_size();
        }
    }

    void GLFW_App::set_window_title(const std::string &the_title)
    {
        for (auto &w : m_windows){ w->set_title(the_title); }
    }

    void GLFW_App::set_cursor_visible(bool b)
    {
        App::set_cursor_visible(b);
        
        for(auto &w : m_windows)
        {
            glfwSetInputMode(w->handle(), GLFW_CURSOR, b ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
        }
    }

    gl::vec2 GLFW_App::cursor_position() const
    {
        gl::vec2 ret;
        if(!windows().empty())
        {
            double x_pos, y_pos;
            glfwGetCursorPos(windows().front()->handle(), &x_pos, &y_pos);
            ret = gl::vec2(x_pos, y_pos);
        }
        return ret;
    }

    void GLFW_App::set_cursor_position(float x, float y)
    {
        if(!windows().empty())
        {
            glfwSetCursorPos(m_windows.front()->handle(), x, y);
        }
    }

    void GLFW_App::poll_events()
    {
        glfwPollEvents();
        ImGui_ImplGlfwGL3_NewFrame();
    }

    void GLFW_App::draw_internal()
    {
        for(auto &w : m_windows){ w->draw(); }
    }

    bool GLFW_App::is_running()
    {
        for(uint32_t i = 0; i < m_windows.size(); i++)
        {
            if(glfwWindowShouldClose(m_windows[i]->handle()))
            {
                m_windows.erase(m_windows.begin() + i);
            }
        }

        return  running() && !m_windows.empty() &&
            !glfwGetKey(m_windows.front()->handle(), GLFW_KEY_ESCAPE );
    }
    
    bool GLFW_App::v_sync() const
    {
        return true;
    }
    
    void GLFW_App::set_v_sync(bool b)
    {
        main_queue().submit([b](){ glfwSwapInterval(b ? 1 : 0); });
    }
    
    double GLFW_App::get_application_time()
    {
        return glfwGetTime();
    }

    void GLFW_App::teardown()
    {
        ImGui_ImplGlfwGL3_Shutdown();
    }

    void GLFW_App::set_fullscreen(bool b, int monitor_index)
    {
        int num;
        GLFWmonitor** monitors = glfwGetMonitors(&num);
        if(m_windows.empty() || monitor_index > num - 1 || monitor_index < 0) return;
        auto m = monitors[monitor_index];

        main_queue().submit([this, b, monitor_index, m]
        {
            // currently not in fullscreen mode
            if(!fullscreen())
            {
                m_win_params = gl::ivec4(m_windows.front()->framebuffer_size(),
                                         m_windows.front()->position());
            }

            const GLFWvidmode* mode = glfwGetVideoMode(m);
            glfwWindowHint(GLFW_RED_BITS, mode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

            gl::ivec2 new_res = b ? gl::ivec2(mode->width, mode->height) : m_win_params.xy();

            GLFW_WindowPtr window = GLFW_Window::create(new_res.x, new_res.y, name(), b, monitor_index,
                                                        m_windows.front()->handle());

            if(!b){ window->set_position(m_win_params.zw()); }
            else{ window->set_position(gl::vec2(0)); }

            // remove first elem from vector
            m_windows.erase(m_windows.begin());
            add_window(window);

            set_window_size(new_res);

            ImGui_ImplGlfwGL3_Init(window->handle(), false, "#version 410");

            gl::reset_state();
            set_cursor_visible(cursor_visible());
            App::set_fullscreen(b, monitor_index);
        });
    }

    int GLFW_App::get_num_monitors() const
    {
        int ret;
        /*GLFWmonitor** monitors = */
        glfwGetMonitors(&ret);
        return ret;
    }

    void GLFW_App::add_window(WindowPtr the_window)
    {
        auto glfw_win = dynamic_pointer_cast<GLFW_Window>(the_window);
        GLFWwindow *w = nullptr;

        if(glfw_win){ w = static_cast<GLFWwindow*>(glfw_win->handle()); }

        if(!w){LOG_ERROR << "add_window failed"; return;}

        glfwSetWindowUserPointer(w, this);
        glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        // static callbacks
        glfwSetMouseButtonCallback(w, &GLFW_App::s_mouseButton);
        glfwSetCursorPosCallback(w, &GLFW_App::s_mouseMove);
        glfwSetScrollCallback(w, &GLFW_App::s_mouseWheel);
        glfwSetKeyCallback(w, &GLFW_App::s_keyFunc);
        glfwSetCharCallback(w, &GLFW_App::s_charFunc);

        main_queue().submit_with_delay([w]()
        {
            // called during resize, move and similar events
            glfwSetWindowRefreshCallback(w, &GLFW_App::s_window_refresh);
        }, 2.f);

        // first added window
        if(m_windows.empty())
        {
            glfwSetWindowSizeCallback(w, &GLFW_App::s_resize);
            the_window->set_draw_function([this]()
            {
                draw();

                // draw tweakbar
                if(display_tweakbar())
                {
                    // console output
                    outstream_gl().draw();

//                    // AntTweakbar
//                    TwDraw();

                    // TODO: remove this, as soon as the side-effect making this necessary is found
                    glDepthMask(GL_TRUE);

                    // render and draw ImGui
                    ImGui::Render();
                    ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
                }
                else{ ImGui::EndFrame(); }
            });
        }

#if GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 1
        glfwSetDropCallback(w, &GLFW_App::s_file_drop_func);
#endif
        m_windows.push_back(glfw_win);
    }

    std::vector<JoystickState> GLFW_App::get_joystick_states() const
    {
        std::vector<JoystickState> ret;
        int count;
        for(int i = GLFW_JOYSTICK_1; i <= GLFW_JOYSTICK_LAST; i++)
        {
            if(!glfwJoystickPresent(i)) continue;

            const float *glfw_axis = glfwGetJoystickAxes(i, &count);
            std::vector<float> axis(glfw_axis, glfw_axis + count);

            const uint8_t *glfw_buttons = glfwGetJoystickButtons(i, &count);
            std::vector<uint8_t> buttons(glfw_buttons, glfw_buttons + count);

            std::string name(glfwGetJoystickName(i));
            ret.emplace_back(name, buttons, axis);
        }
        return ret;
    }

/****************************  Application Events (internal) **************************/

    void GLFW_App::s_error_cb(int error_code, const char* error_msg)
    {
        LOG_WARNING << "GLFW Error (" << error_code << "): " << error_msg;
    }

    void GLFW_App::s_window_refresh(GLFWwindow* window)
    {
        GLFW_App* app = static_cast<GLFW_App*>(glfwGetWindowUserPointer(window));

        for(auto &w : app->windows())
        {
            if(w->handle() == window){ w->draw(); }
        }
        app->swap_buffers();
    }

    void GLFW_App::s_resize(GLFWwindow* window, int w, int h)
    {
        GLFW_App* app = static_cast<GLFW_App*>(glfwGetWindowUserPointer(window));
        app->set_window_size(glm::vec2(w, h));

        // user hook
        if(app->running()) app->resize(w, h);
    }

    void GLFW_App::s_mouseMove(GLFWwindow* window, double x, double y)
    {
        if(!ImGui::GetIO().WantCaptureMouse)
        {
            GLFW_App* app = static_cast<GLFW_App*>(glfwGetWindowUserPointer(window));

            uint32_t buttonModifiers, keyModifiers, bothMods;
            s_getModifiers(window, buttonModifiers, keyModifiers);
            bothMods = buttonModifiers | keyModifiers;
            MouseEvent e(buttonModifiers, (int)x, (int)y, bothMods, glm::ivec2(0));

            if(buttonModifiers){ app->mouse_drag(e); }
            else{ app->mouse_move(e); }
        }
    }

    void GLFW_App::s_mouseButton(GLFWwindow* window, int button, int action, int modifier_mask)
    {
        // ImGUI
        ImGui_ImplGlfw_MouseButtonCallback(window,button, action, modifier_mask);

        if(!ImGui::GetIO().WantCaptureMouse)
        {
            GLFW_App* app = static_cast<GLFW_App*>(glfwGetWindowUserPointer(window));

            uint32_t initiator, keyModifiers, bothMods;
            s_getModifiers(window, initiator, keyModifiers);
            bothMods = initiator | keyModifiers;

            double posX, posY;
            glfwGetCursorPos(window, &posX, &posY);
            MouseEvent e(initiator, (int)posX, (int)posY, bothMods, glm::ivec2(0));

            switch(action)
            {
                case GLFW_PRESS:
                    app->mouse_press(e);
                    break;

                case GLFW_RELEASE:
                    app->mouse_release(e);
                    break;
            }
        }
    }

    void GLFW_App::s_mouseWheel(GLFWwindow* window,double offset_x, double offset_y)
    {
        ImGui_ImplGlfw_ScrollCallback(window, offset_x, offset_y);

        if(!ImGui::GetIO().WantCaptureMouse)
        {
            GLFW_App* app = static_cast<GLFW_App*>(glfwGetWindowUserPointer(window));
            glm::ivec2 offset = glm::ivec2(offset_x, offset_y);

            double posX, posY;
            glfwGetCursorPos(window, &posX, &posY);
            uint32_t buttonMod, keyModifiers = 0;
            s_getModifiers(window, buttonMod, keyModifiers);
            MouseEvent e(0, (int)posX, (int)posY, keyModifiers, offset);
            if(app->running()) app->mouse_wheel(e);
        }
    }

    void GLFW_App::s_keyFunc(GLFWwindow* window, int key, int scancode, int action, int modifier_mask)
    {
        ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, modifier_mask);

        if(!ImGui::GetIO().WantCaptureKeyboard)
        {
            GLFW_App* app = static_cast<GLFW_App*>(glfwGetWindowUserPointer(window));

            uint32_t buttonMod, keyMod;
            s_getModifiers(window, buttonMod, keyMod);

            KeyEvent e(key, key, keyMod);

            switch(action)
            {
                case GLFW_PRESS:
                case GLFW_REPEAT:
                    app->key_press(e);
                    break;

                case GLFW_RELEASE:
                    app->key_release(e);
                    break;


                default:
                    break;
            }
        }
    }

    void GLFW_App::s_charFunc(GLFWwindow* window, unsigned int key)
    {
        ImGui_ImplGlfw_CharCallback(window, key);

        if(!ImGui::GetIO().WantCaptureKeyboard)
        {
//            GLFW_App *app = static_cast<GLFW_App *>(glfwGetWindowUserPointer(window));
//            uint32_t buttonMod, keyMod;
//            s_getModifiers(window, buttonMod, keyMod);
        }
    }

    void GLFW_App::s_getModifiers(GLFWwindow* window,
                                  uint32_t &buttonModifiers,
                                  uint32_t &keyModifiers)
    {
        buttonModifiers = 0;
        if( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) )
            buttonModifiers |= MouseEvent::LEFT_DOWN;
        if( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) )
            buttonModifiers |= MouseEvent::MIDDLE_DOWN;
        if( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) )
            buttonModifiers |= MouseEvent::RIGHT_DOWN;

        keyModifiers = 0;
        if( glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL))
            keyModifiers |= KeyEvent::CTRL_DOWN;
        if( glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT))
            keyModifiers |= KeyEvent::SHIFT_DOWN;
        if( glfwGetKey(window, GLFW_KEY_LEFT_ALT) || glfwGetKey(window, GLFW_KEY_RIGHT_ALT))
            keyModifiers |= KeyEvent::ALT_DOWN;
        if( glfwGetKey(window, GLFW_KEY_LEFT_SUPER) || glfwGetKey(window, GLFW_KEY_RIGHT_SUPER))
            keyModifiers |= KeyEvent::META_DOWN;
    }

    void GLFW_App::s_file_drop_func(GLFWwindow* window, int num_files, const char **paths)
    {
        GLFW_App* app = static_cast<GLFW_App*>(glfwGetWindowUserPointer(window));
        std::vector<std::string> files;

        for(int i = 0; i < num_files; i++)
        {
            files.push_back(paths[i]);
        }
        uint32_t initiator, keyModifiers, bothMods;
        s_getModifiers(window, initiator, keyModifiers);
        bothMods = initiator | keyModifiers;
        double posX, posY;
        glfwGetCursorPos(window, &posX, &posY);
        MouseEvent e(initiator, (int)posX, (int)posY, bothMods, glm::ivec2(0));

        app->file_drop(e, files);
    }

    void GLFW_App::s_monitor_func(GLFWmonitor* the_monitor, int status)
    {
        string name = glfwGetMonitorName(the_monitor);

        if(status == GLFW_CONNECTED){ LOG_DEBUG << "monitor connected: " << name; }
        else if(status == GLFW_DISCONNECTED){ LOG_DEBUG << "monitor disconnected: " << name; }
    }

    void GLFW_App::s_joystick_cb(int joy, int event)
    {
        if (event == GLFW_CONNECTED)
        {
            LOG_DEBUG << "joystick " << joy << " connected";
        }
        else if (event == GLFW_DISCONNECTED)
        {
            LOG_DEBUG << "joystick " << joy << " disconnected";
        }
    }
    
    void GLFW_App::set_display_tweakbar(bool b)
    {
        App::set_display_tweakbar(b);
        
        if(!b)
        {
            bool c = cursor_visible();
            set_cursor_visible(true);
            set_cursor_visible(c);
        }
    }
}
