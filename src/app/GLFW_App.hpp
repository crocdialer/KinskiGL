// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#include "App.hpp"
#include "OutstreamGL.hpp"
#include "imgui/imgui_util.h"

namespace kinski
{
    class MouseEvent;
    class KeyEvent;
    class GLFW_Window;
    
    typedef std::shared_ptr<GLFW_Window> GLFW_WindowPtr;

    class GLFW_Window : public Window
    {
    public:
        static GLFW_WindowPtr create(int width, int height, const std::string &theName,
                                     bool fullscreen, int monitor_index = 0,
                                     GLFWwindow* share = nullptr);
        
        static GLFW_WindowPtr create(int width, int height, const std::string &theName = "KinskiGL");
        
        ~GLFW_Window();
        
        void draw() const override;
        
        gl::ivec2 size() const override;
        void set_size(const gl::ivec2 &the_sz) override;
        gl::ivec2 framebuffer_size() const override;
        
        gl::ivec2 position() const override;
        void set_position(const gl::ivec2 &the_pos) override;
        
        std::string title(const std::string &the_name) const override;
        void set_title(const std::string &the_name) override;
        
        uint32_t monitor_index() const override;
        
        inline GLFWwindow* handle(){return m_handle;};
        
    private:
        GLFW_Window(int width, int height, const std::string &theName, bool fullscreen,
                    int monitor_index, GLFWwindow* share);
        GLFW_Window(int width, int height, const std::string &theName);
        GLFWwindow* m_handle;
        std::string m_title;
    };
    
    class CreateWindowException: public Exception
    {
    public:
        CreateWindowException() :
        Exception(std::string("Could not create GLFW window")){}
    };
    
    class GLFW_App : public App
    {
    public:
        
//        typedef std::shared_ptr<GLFW_App> Ptr;
//        typedef std::weak_ptr<GLFW_App> WeakPtr;
        
        GLFW_App(int argc = 0, char *argv[] = nullptr);
        virtual ~GLFW_App();
        
        void set_window_size(const glm::vec2 &size) override;
        void set_window_title(const std::string &the_title) override;
        void swap_buffers() override;
        double get_application_time() override;
        void teardown() override;
        
        void set_fullscreen(bool b, int monitor_index) override;
        void set_cursor_visible(bool b = true) override;
        void set_cursor_position(float x, float y) override;
        gl::vec2 cursor_position() const override;
        
        bool v_sync() const override;
        void set_v_sync(bool b) override;
        
        std::vector<JoystickState> get_joystick_states() const override;
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void set_display_tweakbar(bool b) override;
        
        int get_num_monitors() const;
        const gl::OutstreamGL& outstream_gl() const {return m_outstream_gl;};
        gl::OutstreamGL& outstream_gl(){return m_outstream_gl;};
        
        // window creation
        void add_window(WindowPtr the_window) override;
        
        const std::vector<GLFW_WindowPtr>& windows() const { return m_windows; }
        
    private:
        
        std::vector<GLFW_WindowPtr> m_windows;
        glm::ivec2 m_lastWheelPos;
        
        //! holds last window size and position, when in fullscreen mode
        glm::ivec4 m_win_params;
        
        // internal initialization. performed when run is invoked
        void init() override;
        void poll_events() override;
        void draw_internal() override;
        bool is_running() override;
        
        // GLFW static callbacks
        static void s_error_cb(int error_code, const char* error_msg);
        static void s_window_refresh(GLFWwindow* window);
        static void s_resize(GLFWwindow* window, int w, int h);
        static void s_mouseMove(GLFWwindow* window,double x, double y);
        static void s_mouseButton(GLFWwindow* window,int button, int action, int modifier_mask);
        static void s_mouseWheel(GLFWwindow* window,double offset_x, double offset_y);        
        static void s_keyFunc(GLFWwindow* window, int key, int scancode, int action, int modifier_mask);
        static void s_charFunc(GLFWwindow* window, unsigned int key);
        static void s_file_drop_func(GLFWwindow* window, int num_files, const char **paths);
        static void s_monitor_func(GLFWmonitor* the_monitor, int);
        static void s_joystick_cb(int joy, int event);

        // return the current key and mouse button modifier mask
        static void s_getModifiers(GLFWwindow* window, uint32_t &buttonModifiers,
                                   uint32_t &keyModifiers);
        
        gl::OutstreamGL m_outstream_gl;
    };
}
