// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "gl/gl.hpp"
#include "gl/Material.hpp"
#include "GLFW_App.h"
#include "AntTweakBarConnector.h"

using namespace std;

namespace kinski
{
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

    gl::vec2 GLFW_Window::size() const
    {
        int w, h;
        glfwGetWindowSize(m_handle, &w, &h);
        return gl::vec2(w, h);
    }
    
    gl::vec2 GLFW_Window::position() const
    {
        int x, y;
        glfwGetWindowPos(m_handle, &x, &y);
        return gl::vec2(x, y);
    }
    
    void GLFW_Window::set_position(const gl::vec2 &the_pos)
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

    gl::vec2 GLFW_Window::framebuffer_size() const
    {
        int w, h;
        glfwGetFramebufferSize(m_handle, &w, &h);
        return gl::vec2(w, h);
    }

    void GLFW_Window::draw() const
    {
        glfwMakeContextCurrent(m_handle);

        gl::set_window_dimension(framebuffer_size());

        glDepthMask(GL_TRUE);

        if(m_draw_function){ m_draw_function(); }
    }

    GLFW_App::GLFW_App(const int width, const int height):
    App(width, height),
    m_lastWheelPos(0)
    {
        glfwSetErrorCallback(&s_error_cb);
    }

    GLFW_App::~GLFW_App()
    {
        TwTerminate();

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
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 4);

        // create the window
        auto main_window = GLFW_Window::create(getWidth(), getHeight(), name(), fullscreen());
        add_window(main_window);

        // set graphical log stream
        Logger::get()->add_outstream(&m_outstream_gl);

        // version
        LOG_INFO<<"OpenGL: " << glGetString(GL_VERSION);
        LOG_INFO<<"GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION);

        glfwSwapInterval(1);
        glClearColor(0, 0, 0, 1);

        // file search paths
        kinski::add_search_path(".", true);
        kinski::add_search_path("./res", true);
        kinski::add_search_path("../Resources", true);

        //---------------------------------
#if defined(KINSKI_MAC)
        kinski::add_search_path("/Library/Fonts");
        kinski::add_search_path("~/Library/Fonts");
#elif defined(KINSKI_LINUX)
        kinski::add_search_path("~/.local/share/fonts");
#endif
        //---------------------------------

        // AntTweakbar
        TwInit(TW_OPENGL_CORE, NULL);
        TwWindowSize(getWidth(), getHeight());

        // call user defined setup callback
        setup();
    }

    void GLFW_App::swapBuffers()
    {
        for(const auto &window : m_windows)
        {
            glfwSwapBuffers(window->handle());
        }
    }

    void GLFW_App::set_window_size(const glm::vec2 &size)
    {
        App::set_window_size(size);
        gl::set_window_dimension(size);
        TwWindowSize(size.x, size.y);
        if(!m_windows.empty())
            glfwSetWindowSize(m_windows.front()->handle(), (int)size[0], (int)size[1]);

    }

    void GLFW_App::set_window_title(const std::string &the_title)
    {
        for (auto &w : m_windows){ w->set_title(the_title); }
    }

    void GLFW_App::setCursorVisible(bool b)
    {
        App::setCursorVisible(b);
        glfwSetInputMode(m_windows.front()->handle(), GLFW_CURSOR,
                         b ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
    }

    void GLFW_App::setCursorPosition(float x, float y)
    {
        glfwSetCursorPos(m_windows.front()->handle(), x, y);
    }

    void GLFW_App::pollEvents()
    {
        glfwPollEvents();
    }

    void GLFW_App::draw_internal()
    {
        for(auto &w : m_windows){ w->draw(); }
    }

    bool GLFW_App::checkRunning()
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

    double GLFW_App::getApplicationTime()
    {
        return glfwGetTime();
    }

    void GLFW_App::set_fullscreen(bool b, int monitor_index)
    {
        if(m_windows.empty()) return;

        int num;
        GLFWmonitor** monitors = glfwGetMonitors(&num);

        monitor_index = clamp(monitor_index, 0, num - 1);
        auto m = monitors[monitor_index];
//        const GLFWvidmode* video_modes = glfwGetVideoModes(m, &num);

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
        
        // remove first elem from vector
        m_windows.erase(m_windows.begin());
        add_window(window);

        s_resize(window->handle(), new_res.x, new_res.y);
        gl::apply_material(gl::Material::create());
        App::set_fullscreen(b, monitor_index);
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

        // called during resize, move and similar events
//        glfwSetWindowRefreshCallback(w, &GLFW_App::s_window_refresh);

        // first added window
        if(m_windows.empty())
        {
            glfwSetWindowSizeCallback(w, &GLFW_App::s_resize);
            the_window->set_draw_function([this]()
            {
                gl::clear();
                draw();

                // draw tweakbar
                if(displayTweakBar())
                {
                    // console output
                    outstream_gl().draw();
                    TwDraw();
                }
            });
//            resize(glfw_win->size().x, glfw_win->size().y);
        }

#if GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 1
        glfwSetDropCallback(w, &GLFW_App::s_file_drop_func);
#endif
        m_windows.push_back(glfw_win);
        
//        glfwMakeContextCurrent(w);
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
            ret.push_back(JoystickState(name, buttons, axis));
        }
        return ret;
    }

/****************************  Application Events (internal) **************************/

    void GLFW_App::s_error_cb(int error_code, const char* error_msg)
    {
        LOG_ERROR<<"GLFW Error ("<< error_code <<"): "<<error_msg;
    }

    void GLFW_App::s_window_refresh(GLFWwindow* window)
    {
        GLFW_App* app = static_cast<GLFW_App*>(glfwGetWindowUserPointer(window));

        LOG_TRACE << "window refresh";

        for(auto &w : app->windows())
        {
            if(w->handle() == window){ w->draw(); }
        }
        app->swapBuffers();
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
        GLFW_App* app = static_cast<GLFW_App*>(glfwGetWindowUserPointer(window));
        if(app->displayTweakBar() && app->windows().front()->handle() == window)
            TwEventMousePosGLFW((int)x, (int)y);
        uint32_t buttonModifiers, keyModifiers, bothMods;
        s_getModifiers(window, buttonModifiers, keyModifiers);
        bothMods = buttonModifiers | keyModifiers;
        MouseEvent e(buttonModifiers, (int)x, (int)y, bothMods, glm::ivec2(0));

        if(buttonModifiers)
            app->mouseDrag(e);
        else
            app->mouseMove(e);
    }

    void GLFW_App::s_mouseButton(GLFWwindow* window, int button, int action, int modifier_mask)
    {
        GLFW_App* app = static_cast<GLFW_App*>(glfwGetWindowUserPointer(window));
        if(app->displayTweakBar() && app->windows().front()->handle() == window)
            TwEventMouseButtonGLFW(button, action);

        uint32_t initiator, keyModifiers, bothMods;
        s_getModifiers(window, initiator, keyModifiers);
        bothMods = initiator | keyModifiers;

        double posX, posY;
        glfwGetCursorPos(window, &posX, &posY);

        MouseEvent e(initiator, (int)posX, (int)posY, bothMods, glm::ivec2(0));

        switch(action)
        {
            case GLFW_PRESS:
                app->mousePress(e);
                break;

            case GLFW_RELEASE:
                app->mouseRelease(e);
                break;
        }
    }

    void GLFW_App::s_mouseWheel(GLFWwindow* window,double offset_x, double offset_y)
    {
        GLFW_App* app = static_cast<GLFW_App*>(glfwGetWindowUserPointer(window));
        glm::ivec2 offset = glm::ivec2(offset_x, offset_y);
        app->m_lastWheelPos -= offset;
        if(app->displayTweakBar() && app->windows().front()->handle() == window)
            TwMouseWheel(app->m_lastWheelPos.y);

        double posX, posY;
        glfwGetCursorPos(window, &posX, &posY);
        uint32_t buttonMod, keyModifiers = 0;
        s_getModifiers(window, buttonMod, keyModifiers);
        MouseEvent e(0, (int)posX, (int)posY, keyModifiers, offset);
        if(app->running()) app->mouseWheel(e);
    }

    void GLFW_App::s_keyFunc(GLFWwindow* window, int key, int scancode, int action, int modifier_mask)
    {
        GLFW_App* app = static_cast<GLFW_App*>(glfwGetWindowUserPointer(window));
        if(app->displayTweakBar() && app->windows().front()->handle() == window)
            TwEventKeyGLFW(key, action);

        uint32_t buttonMod, keyMod;
        s_getModifiers(window, buttonMod, keyMod);

        KeyEvent e(key, key, keyMod);

        switch(action)
        {
            case GLFW_PRESS:
            case GLFW_REPEAT:
                app->keyPress(e);
                break;

            case GLFW_RELEASE:
                app->keyRelease(e);
                break;


            default:
                break;
        }
    }

    void GLFW_App::s_charFunc(GLFWwindow* window, unsigned int key)
    {
        GLFW_App* app = static_cast<GLFW_App*>(glfwGetWindowUserPointer(window));
        if(app->displayTweakBar() && app->windows().front()->handle() == window)
            TwEventCharGLFW(key, GLFW_PRESS);

        if(key == GLFW_KEY_SPACE){return;}

        uint32_t buttonMod, keyMod;
        s_getModifiers(window, buttonMod, keyMod);

//        KeyEvent e(0, key, keyMod);
//        app->keyPress(e);
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

        app->fileDrop(e, files);
    }

/****************************  TweakBar + Properties **************************/

    void GLFW_App::add_tweakbar_for_component(const Component::Ptr &the_component)
    {
        if(!the_component) return;
        auto tw_bar = TwNewBar(the_component->name().c_str());
        m_tweakBars[the_component] = tw_bar;

        setBarColor(glm::vec4(0, 0, 0, .5), tw_bar);
        setBarSize(glm::ivec2(250, 500));
        glm::ivec2 offset(10);
        setBarPosition(glm::ivec2(offset.x + 260 * (m_tweakBars.size() - 1), offset.y), tw_bar);
        addPropertyListToTweakBar(the_component->get_property_list(), "", tw_bar);
    }

    void GLFW_App::remove_tweakbar_for_component(const Component::Ptr &the_component)
    {
        auto it = m_tweakBars.find(the_component);

        if(it != m_tweakBars.end())
        {
            TwDeleteBar(it->second);
            m_tweakBars.erase(it);
        }
    }

    void GLFW_App::addPropertyToTweakBar(const Property::Ptr propPtr,
                                         const string &group,
                                         TwBar *theBar)
    {
        if(!theBar)
        {
            if(m_tweakBars.empty()){ return; }
            theBar = m_tweakBars[shared_from_this()];
        }

        try { AntTweakBarConnector::connect(theBar, propPtr, group); }
        catch (AntTweakBarConnector::PropertyUnsupportedException &e){ LOG_ERROR<<e.what(); }
    }

    void GLFW_App::addPropertyListToTweakBar(const list<Property::Ptr> &theProps,
                                             const string &group,
                                             TwBar *theBar)
    {
        if(!theBar)
        {
            if(m_tweakBars.empty()){ return; }
            theBar = m_tweakBars[shared_from_this()];
        }
        for (const auto &property : theProps)
        {
            addPropertyToTweakBar(property, group, theBar);
        }
        TwAddSeparator(theBar, "sep1", NULL);
    }

    void GLFW_App::setBarPosition(const glm::ivec2 &thePos, TwBar *theBar)
    {
        if(!theBar)
        {
            if(m_tweakBars.empty()){ return; }
            theBar = m_tweakBars[shared_from_this()];
        }
        std::stringstream ss;
        ss << TwGetBarName(theBar) << " position='" <<thePos.x
        <<" " << thePos.y <<"'";
        TwDefine(ss.str().c_str());
    }

    void GLFW_App::setBarSize(const glm::ivec2 &theSize, TwBar *theBar)
    {
        if(!theBar)
        {
            if(m_tweakBars.empty()){ return; }
            theBar = m_tweakBars[shared_from_this()];
        }
        std::stringstream ss;
        ss << TwGetBarName(theBar) << " size='" <<theSize.x
        <<" " << theSize.y <<"'";
        TwDefine(ss.str().c_str());
    }

    void GLFW_App::setBarColor(const glm::vec4 &theColor, TwBar *theBar)
    {
        if(!theBar)
        {
            if(m_tweakBars.empty()){ return; }
            theBar = m_tweakBars[shared_from_this()];
        }
        std::stringstream ss;
        glm::ivec4 color(theColor * 255.f);
        ss << TwGetBarName(theBar) << " color='" <<color.r
        <<" " << color.g << " " << color.b <<"' alpha="<<color.a;
        TwDefine(ss.str().c_str());
    }

    void GLFW_App::setBarTitle(const std::string &theTitle, TwBar *theBar)
    {
        if(!theBar)
        {
            if(m_tweakBars.empty()){ return; }
            theBar = m_tweakBars[shared_from_this()];
        }
        std::stringstream ss;
        ss << TwGetBarName(theBar) << " label='" << theTitle <<"'";
        TwDefine(ss.str().c_str());
    }
}
