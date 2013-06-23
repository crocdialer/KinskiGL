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
#include <AntTweakBar.h>
#include "App.h"

namespace kinski
{
    class MouseEvent;
    class KeyEvent;
    
    class GLFW_Window
    {
    public:
        typedef std::shared_ptr<GLFW_Window> Ptr;
        GLFW_Window(int width, int height, const std::string &theName = "KinskiGL");
        ~GLFW_Window();
        inline GLFWwindow* handle(){return m_handle;};
    private:
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
        
        void setWindowSize(const glm::vec2 size);
        void swapBuffers();
        double getApplicationTime();
        
        void setFullSceen(bool b = true);
        
        ///////////////////////////////////////////////////////////////////////////////
        
        void set_displayTweakBar(bool b){m_displayTweakBar = b;};
        bool displayTweakBar() const {return m_displayTweakBar;};
        
        void addPropertyToTweakBar(const Property::Ptr propPtr,
                                   const std::string &group = "",
                                   TwBar *theBar = NULL);
        
        void addPropertyListToTweakBar(const std::list<Property::Ptr> &theProps,
                                       const std::string &group = "",
                                       TwBar *theBar = NULL);
        
        void setBarPosition(const glm::ivec2 &thePos, TwBar *theBar = NULL);
        void setBarSize(const glm::ivec2 &theSize, TwBar *theBar = NULL);
        void setBarColor(const glm::vec4 &theColor, TwBar *theBar = NULL);
        void setBarTitle(const std::string &theTitle, TwBar *theBar = NULL);
        
        const std::map<TwBar*, std::list<Property::Ptr> >& 
        getTweakProperties() const {return m_tweakProperties;};
        
        const std::vector<TwBar*>& tweakBars() const { return m_tweakBars; };
        std::vector<TwBar*>& tweakBars() { return m_tweakBars; };
        
        void create_tweakbar_from_component(const Component::Ptr & the_component);
        
    private:
        
        std::vector<GLFW_Window::Ptr> m_windows;
        glm::ivec2 m_lastWheelPos;
        
        // internal initialization. performed when run is invoked
        void init();
        void draw_internal();
        bool checkRunning();
        
        // GLFW static callbacks
        static void s_resize(GLFWwindow* window, int w, int h);
        static void s_mouseMove(GLFWwindow* window,double x, double y);
        static void s_mouseButton(GLFWwindow* window,int button, int action, int modifier_mask);
        static void s_mouseWheel(GLFWwindow* window,double offset_x, double offset_y);        
        static void s_keyFunc(GLFWwindow* window, int key, int scancode, int action, int modifier_mask);
        static void s_charFunc(GLFWwindow* window, unsigned int key);

        // return the current key and mouse button modifier mask
        static void s_getModifiers(GLFWwindow* window, int modifier_mask, uint32_t &buttonModifiers,
                                   uint32_t &keyModifiers);
        
        std::vector<TwBar*> m_tweakBars;
        bool m_displayTweakBar;
        
        std::map<TwBar*, std::list<Property::Ptr> > m_tweakProperties;
    };
}
#endif // _KINSKI_GLFW_APP_IS_INCLUDED_