// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "gl/KinskiGL.h"
#include "GLFW_App.h"
#include "core/file_functions.h"
#include "AntTweakBarConnector.h"

using namespace std;

namespace kinski
{
    GLFW_Window::GLFW_Window(int width, int height, const std::string &theName, bool fullscreen,
                             int monitor_index, GLFWwindow* share)
     {
         int monitor_count = 0;
         GLFWmonitor **monitors = glfwGetMonitors(&monitor_count);
         monitor_index = clamp(monitor_index, 0, monitor_count - 1);
//         GLFWmonitor *monitor_handle = monitor_count > 1 ? monitors[monitor_index] : glfwGetPrimaryMonitor();
         
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
    
    GLFW_Window::~GLFW_Window()
    {
        glfwDestroyWindow(m_handle);
    }
    
    GLFW_App::GLFW_App(const int width, const int height):
    App(width, height),
    m_lastWheelPos(0),
    m_displayTweakBar(true)
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
        
        //TODO: find out why this is necessary for smooth gradients
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
        addWindow(GLFW_Window::create(getWidth(), getHeight(), getName(), fullSceen()));
        gl::setWindowDimension(windowSize());
        
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
        #ifdef KINSKI_MAC
        kinski::add_search_path("/Library/Fonts");
        kinski::add_search_path("~/Library/Fonts");
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
    
    void GLFW_App::setWindowSize(const glm::vec2 &size)
    {
        App::setWindowSize(size);
        TwWindowSize(size.x, size.y);
        if(!m_windows.empty())
            glfwSetWindowSize(m_windows.back()->handle(), (int)size[0], (int)size[1]);
        
    }
    
    void GLFW_App::setCursorVisible(bool b)
    {
        App::setCursorVisible(b);
        glfwSetInputMode(m_windows.back()->handle(), GLFW_CURSOR,
                         b ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
    }
    
    void GLFW_App::setCursorPosition(float x, float y)
    {
        glfwSetCursorPos(m_windows.back()->handle(), x, y);
    }
    
    void GLFW_App::pollEvents()
    {
        glfwPollEvents();
    }
    
    void GLFW_App::draw_internal()
    {
        glDepthMask(GL_TRUE);
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        draw();
        
        // draw tweakbar
        if(m_displayTweakBar)
        {
            // console output
            outstream_gl().draw();
            
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            TwDraw();
        }
    }
    
    bool GLFW_App::checkRunning()
    {
        return  running() &&
                !glfwGetKey(m_windows.back()->handle(), GLFW_KEY_ESCAPE ) &&
                !glfwWindowShouldClose(m_windows.back()->handle());
    }
    
    double GLFW_App::getApplicationTime()
    {
        return glfwGetTime();
    }
    
    void GLFW_App::setFullSceen(bool b)
    {
        setFullSceen(b, 0);
    }
    
    void GLFW_App::setFullSceen(bool b, int monitor_index)
    {
        App::setFullSceen(b);
        
        if(m_windows.empty()) return;
        
        GLFW_WindowPtr window = GLFW_Window::create(getWidth(), getHeight(), getName(), b, monitor_index,
                                                    m_windows.back()->handle());
        m_windows.clear();
        addWindow(window);
        
        int w, h;
        glfwGetFramebufferSize(window->handle(), &w, &h);
        setWindowSize(glm::vec2(w, h));
    }
    
    int GLFW_App::get_num_monitors() const
    {
        int ret;
        /*GLFWmonitor** monitors = */
        glfwGetMonitors(&ret);
        return ret;
    }
    
    void GLFW_App::addWindow(const GLFW_WindowPtr &the_window)
    {
        m_windows.push_back(the_window);
        glfwSetWindowUserPointer(the_window->handle(), this);
        glfwSetInputMode(m_windows.back()->handle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        
        // static callbacks
        glfwSetMouseButtonCallback(the_window->handle(), &GLFW_App::s_mouseButton);
        glfwSetCursorPosCallback(the_window->handle(), &GLFW_App::s_mouseMove);
        glfwSetScrollCallback(the_window->handle(), &GLFW_App::s_mouseWheel);
        glfwSetKeyCallback(the_window->handle(), &GLFW_App::s_keyFunc);
        glfwSetCharCallback(the_window->handle(), &GLFW_App::s_charFunc);
        glfwSetWindowSizeCallback(the_window->handle(), &GLFW_App::s_resize);
        
#if GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 1
        glfwSetDropCallback(the_window->handle(), &GLFW_App::s_file_drop_func);
#endif
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
    
    void GLFW_App::s_resize(GLFWwindow* window, int w, int h)
    {
        GLFW_App* app = static_cast<GLFW_App*>(glfwGetWindowUserPointer(window));
        app->setWindowSize(glm::vec2(w, h));
        glViewport(0, 0, w, h);
        gl::setWindowDimension(app->windowSize());
        TwWindowSize(w, h);
        
        // user hook
        if(app->running()) app->resize(w, h);
    }
    
    void GLFW_App::s_mouseMove(GLFWwindow* window, double x, double y)
    {
        GLFW_App* app = static_cast<GLFW_App*>(glfwGetWindowUserPointer(window));
        if(app->displayTweakBar())
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
    
    void GLFW_App::s_mouseButton(GLFWwindow* window,int button, int action, int modifier_mask)
    {
        GLFW_App* app = static_cast<GLFW_App*>(glfwGetWindowUserPointer(window));
        if(app->displayTweakBar())
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
        if(app->displayTweakBar())
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
        if(app->displayTweakBar())
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
        if(app->displayTweakBar())
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
    
    void GLFW_App::s_file_drop_func(GLFWwindow* window, int num_files,const char **paths)
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
    
    void GLFW_App::create_tweakbar_from_component(const Component::Ptr &the_component)
    {
        if(!the_component) return;
        m_tweakBars.push_back(TwNewBar(the_component->getName().c_str()));
        setBarColor(glm::vec4(0, 0, 0, .5), m_tweakBars.back());
        setBarSize(glm::ivec2(250, 500));
        glm::ivec2 offset(10);
        setBarPosition(glm::ivec2(offset.x + 260 * (m_tweakBars.size() - 1), offset.y), m_tweakBars.back());
        addPropertyListToTweakBar(the_component->getPropertyList(), "", m_tweakBars.back());
    }
    
    void GLFW_App::addPropertyToTweakBar(const Property::Ptr propPtr,
                                         const string &group,
                                         TwBar *theBar)
    {
        if(!theBar)
        {   if(m_tweakBars.empty()) return;
            theBar = m_tweakBars.front();
        }
        m_tweakProperties[theBar].push_back(propPtr);
        
        try {
            AntTweakBarConnector::connect(theBar, propPtr, group);
        } catch (AntTweakBarConnector::PropertyUnsupportedException &e) {
            LOG_ERROR<<e.what();
        }
    }
    
    void GLFW_App::addPropertyListToTweakBar(const list<Property::Ptr> &theProps,
                                             const string &group,
                                             TwBar *theBar)
    {
        if(!theBar)
        {   if(m_tweakBars.empty()) return;
            theBar = m_tweakBars.front();
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
        {   if(m_tweakBars.empty()) return;
            theBar = m_tweakBars.front();
        }
        std::stringstream ss;
        ss << TwGetBarName(theBar) << " position='" <<thePos.x
        <<" " << thePos.y <<"'";
        TwDefine(ss.str().c_str());
    }
    
    void GLFW_App::setBarSize(const glm::ivec2 &theSize, TwBar *theBar)
    {
        if(!theBar)
        {   if(m_tweakBars.empty()) return;
            theBar = m_tweakBars.front();
        }
        std::stringstream ss;
        ss << TwGetBarName(theBar) << " size='" <<theSize.x
        <<" " << theSize.y <<"'";
        TwDefine(ss.str().c_str());
    }
    
    void GLFW_App::setBarColor(const glm::vec4 &theColor, TwBar *theBar)
    {
        if(!theBar)
        {   if(m_tweakBars.empty()) return;
            theBar = m_tweakBars.front();
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
        {   if(m_tweakBars.empty()) return;
            theBar = m_tweakBars.front();
        }
        std::stringstream ss;
        ss << TwGetBarName(theBar) << " label='" << theTitle <<"'";
        TwDefine(ss.str().c_str());
    }
}
