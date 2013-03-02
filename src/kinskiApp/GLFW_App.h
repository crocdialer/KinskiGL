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

#include "App.h"

#define GLFW_INCLUDE_GL3
#define GLFW_NO_GLU
#include <GL/glfw.h>
#include <AntTweakBar.h>

namespace kinski
{
    class MouseEvent;
    class KeyEvent;
    
    class GLFW_App : public App
    {
    public:
        
        typedef std::shared_ptr<GLFW_App> Ptr;
        typedef std::weak_ptr<GLFW_App> WeakPtr;
        
        static Ptr getInstance();
        
        GLFW_App(const int width = 800, const int height = 600);
        virtual ~GLFW_App();
        
        
        void setWindowSize(const glm::vec2 size);
        void swapBuffers();
        double getApplicationTime();
        
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
        
        const std::map<TwBar*, Property::Ptr>& 
        getTweakProperties() const {return m_tweakProperties;};
        
    private:
        
        // internal initialization. performed when run is invoked
        void init();
        void draw_internal();
        bool checkRunning();
        
        // GLFW static callbacks
        static void s_resize(int w, int h){getInstance()->__resize(w, h);};
        static void s_mouseMove(int x, int y){getInstance()->__mouseMove(x, y);};
        static void s_mouseButton(int button, int action){getInstance()->__mouseButton(button, action);};
        static void s_mouseWheel(int pos){getInstance()->__mouseWheel(pos);};
        
        static void s_keyFunc(int key, int action){getInstance()->__keyFunc(key, action);};
        static void s_charFunc(int key, int action){getInstance()->__charFunc(key, action);};
        
        void __resize(int w,int h);
        void __mouseMove(int x,int y);
        void __mouseButton(int button, int action);
        void __mouseWheel(int pos);
        
        void __keyFunc(int key, int action);
        void __charFunc(int key, int action);
        
        // return the current key and mouse button modifier mask
        void getModifiers(uint32_t &buttonModifiers, uint32_t &keyModifiers);
        
        int m_lastWheelPos;
        static WeakPtr s_instance;
        
        std::list<TwBar*> m_tweakBarList;
        bool m_displayTweakBar;
        
        std::map<TwBar*, Property::Ptr> m_tweakProperties;
        
    };
}
#endif // _KINSKI_GLFW_APP_IS_INCLUDED_