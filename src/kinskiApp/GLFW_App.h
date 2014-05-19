// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef _KINSKI_GLFW_APP_IS_INCLUDED_
#define _KINSKI_GLFW_APP_IS_INCLUDED_

#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
//#include <GLFW/glfw3native.h>
#include "App.h"
#include "OutstreamGL.h"

struct CTwBar;

namespace kinski
{
    class MouseEvent;
    class KeyEvent;
    class GLFW_Window;
    
    typedef std::shared_ptr<GLFW_Window> GLFW_WindowPtr;
    
    class GLFW_Window
    {
    public:
        
        static GLFW_WindowPtr create(int width, int height, const std::string &theName,
                                     bool fullscreen, GLFWwindow* share = NULL)
        {
            return GLFW_WindowPtr(new GLFW_Window(width, height, theName, fullscreen, share));
        }
        
        static GLFW_WindowPtr create(int width, int height, const std::string &theName = "KinskiGL")
        {
            return GLFW_WindowPtr(new GLFW_Window(width, height, theName));
        }
        
        ~GLFW_Window();
        inline GLFWwindow* handle(){return m_handle;};
    private:
        GLFW_Window(int width, int height, const std::string &theName, bool fullscreen,
                    GLFWwindow* share);
        GLFW_Window(int width, int height, const std::string &theName);
        GLFWwindow* m_handle;
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
        
        typedef std::shared_ptr<GLFW_App> Ptr;
        typedef std::weak_ptr<GLFW_App> WeakPtr;
        
        GLFW_App(const int width = 800, const int height = 600);
        virtual ~GLFW_App();
        
        void setWindowSize(const glm::vec2 &size);
        void swapBuffers();
        double getApplicationTime();
        
        void setFullSceen(bool b = true);
        void setCursorVisible(bool b = true);
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void displayTweakBar(bool b){m_displayTweakBar = b;};
        bool displayTweakBar() const {return m_displayTweakBar;};
        
        void addPropertyToTweakBar(const Property::Ptr propPtr,
                                   const std::string &group = "",
                                   CTwBar *theBar = NULL);
        
        void addPropertyListToTweakBar(const std::list<Property::Ptr> &theProps,
                                       const std::string &group = "",
                                       CTwBar *theBar = NULL);
        
        void setBarPosition(const glm::ivec2 &thePos, CTwBar *theBar = NULL);
        void setBarSize(const glm::ivec2 &theSize, CTwBar *theBar = NULL);
        void setBarColor(const glm::vec4 &theColor, CTwBar *theBar = NULL);
        void setBarTitle(const std::string &theTitle, CTwBar *theBar = NULL);
        
        const std::map<CTwBar*, std::list<Property::Ptr> >& 
        getTweakProperties() const {return m_tweakProperties;};
        
        const std::vector<CTwBar*>& tweakBars() const { return m_tweakBars; };
        std::vector<CTwBar*>& tweakBars() { return m_tweakBars; };
        
        void create_tweakbar_from_component(const Component::Ptr & the_component);
        
        int get_num_monitors() const;
        const gl::OutstreamGL& outstream_gl() const {return m_outstream_gl;};
        gl::OutstreamGL& outstream_gl(){return m_outstream_gl;};
        
    private:
        
        std::vector<GLFW_WindowPtr> m_windows;
        glm::ivec2 m_lastWheelPos;
        
        // internal initialization. performed when run is invoked
        void init();
        void pollEvents();
        void draw_internal();
        bool checkRunning();
        
        // GLFW internal
        void addWindow(const GLFW_WindowPtr &the_window);
        
        // GLFW static callbacks
        static void s_error_cb(int error_code, const char* error_msg);
        static void s_resize(GLFWwindow* window, int w, int h);
        static void s_mouseMove(GLFWwindow* window,double x, double y);
        static void s_mouseButton(GLFWwindow* window,int button, int action, int modifier_mask);
        static void s_mouseWheel(GLFWwindow* window,double offset_x, double offset_y);        
        static void s_keyFunc(GLFWwindow* window, int key, int scancode, int action, int modifier_mask);
        static void s_charFunc(GLFWwindow* window, unsigned int key);

        // return the current key and mouse button modifier mask
        static void s_getModifiers(GLFWwindow* window, int modifier_mask, uint32_t &buttonModifiers,
                                   uint32_t &keyModifiers);
        
        std::vector<CTwBar*> m_tweakBars;
        bool m_displayTweakBar;
        
        std::map<CTwBar*, std::list<Property::Ptr> > m_tweakProperties;
        
        gl::OutstreamGL m_outstream_gl;
    };
}
#endif // _KINSKI_GLFW_APP_IS_INCLUDED_