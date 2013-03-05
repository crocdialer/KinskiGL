// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "kinskiGL/KinskiGL.h"
#ifndef KINSKI_RASPI 
#include "GLFW_App.h"
#include "kinskiCore/file_functions.h"
#include "AntTweakBarConnector.h"

using namespace std;

namespace kinski
{
    GLFW_App::WeakPtr GLFW_App::s_instance;
    
    GLFW_App::GLFW_App(const int width, const int height):
    App(width, height),
    m_lastWheelPos(0),
    m_displayTweakBar(true)
    {
        
    }
    
    GLFW_App::~GLFW_App()
    {
        TwTerminate();
        
        // Close window and terminate GLFW
        glfwTerminate();
    }
    
    GLFW_App::Ptr GLFW_App::getInstance()
    {
        return s_instance.lock();
    }
    
    void GLFW_App::init()
    {
        s_instance = dynamic_pointer_cast<GLFW_App>(shared_from_this());
        
        // Initialize GLFW
        if( !glfwInit() )
        {
            throw exception();
        }
        
        // request an OpenGl 3.2 Context
        glfwOpenWindowHint( GLFW_OPENGL_VERSION_MAJOR, 3 );    
        glfwOpenWindowHint( GLFW_OPENGL_VERSION_MINOR, 2 );
        glfwOpenWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwOpenWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 4);
        
        // Open an OpenGL window
        if( !glfwOpenWindow( getWidth(), getHeight(), 0, 0, 0, 0, 24, 0,
                             fullSceen() ? GLFW_FULLSCREEN : GLFW_WINDOW ) )
        {
            glfwTerminate();
            throw Exception("Could not init OpenGL window");
        }
        
        // show mouse cursor in fullscreen ?
        if(fullSceen() && cursorVisible()) glfwEnable(GLFW_MOUSE_CURSOR);
        
        glfwSwapInterval(1);
        glClearColor(0, 0, 0, 1);
        
        // version
        LOG_INFO<<"OpenGL: " << glGetString(GL_VERSION);
        LOG_INFO<<"GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION);
        
        // file search paths
        kinski::addSearchPath("./");
        kinski::addSearchPath("./res/");
        kinski::addSearchPath("../Resources/");
        
        // AntTweakbar
        TwInit(TW_OPENGL_CORE, NULL);
        TwWindowSize(getWidth(), getHeight());
        
        m_tweakBarList.push_back(TwNewBar(getName().c_str()));
        
        glfwSetMouseButtonCallback(&s_mouseButton);
        glfwSetMousePosCallback(&s_mouseMove);
        glfwSetMouseWheelCallback(&s_mouseWheel);
        
        glfwSetKeyCallback(&s_keyFunc);
        glfwSetCharCallback(&s_charFunc);
        
        // send window size events to AntTweakBar
        glfwSetWindowSizeCallback(&s_resize);
        
        // call user defined setup callback
        setup();
        
        // activate property observer mechanism
        observeProperties();
    }
    
    void GLFW_App::swapBuffers()
    {
        glfwSwapBuffers();
    }
    
    void GLFW_App::setWindowSize(const glm::vec2 size)
    {
        App::setWindowSize(size);
        glfwSetWindowSize(size[0], size[1]);
    }
    
    void GLFW_App::draw_internal()
    {
        draw();
        
        // draw tweakbar
        if(m_displayTweakBar)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            TwDraw();
        }
    }
    
    bool GLFW_App::checkRunning()
    {
        return !glfwGetKey( GLFW_KEY_ESC ) && glfwGetWindowParam( GLFW_OPENED );
    }
    
    double GLFW_App::getApplicationTime()
    {
        return glfwGetTime();
    }
    
/****************************  Application Events (internal) **************************/
    
    void GLFW_App::__resize(int w,int h)
    {
        setWindowSize(glm::vec2(w, h));
        glViewport(0, 0, w, h);
        gl::setWindowDimension(windowSize());
        TwWindowSize(w, h);
        
        // user hook
        if(running()) resize(w, h);
    }
    
    void GLFW_App::getModifiers(uint32_t &buttonModifiers, uint32_t &keyModifiers)
    {
        buttonModifiers = 0;
        if( glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT) )
            buttonModifiers |= MouseEvent::LEFT_DOWN;
        if( glfwGetMouseButton(GLFW_MOUSE_BUTTON_MIDDLE) )
            buttonModifiers |= MouseEvent::MIDDLE_DOWN;
        if( glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT) )
            buttonModifiers |= MouseEvent::RIGHT_DOWN;
        
        keyModifiers = 0;
        if( glfwGetKey(GLFW_KEY_LCTRL) || glfwGetKey(GLFW_KEY_RCTRL))
            keyModifiers |= KeyEvent::CTRL_DOWN;
        if( glfwGetKey(GLFW_KEY_LSHIFT) || glfwGetKey(GLFW_KEY_RSHIFT))
            keyModifiers |= KeyEvent::SHIFT_DOWN;
        if( glfwGetKey(GLFW_KEY_LALT) || glfwGetKey(GLFW_KEY_RALT))
            keyModifiers |= KeyEvent::ALT_DOWN;
        if( glfwGetKey(GLFW_KEY_LSUPER))
            keyModifiers |= KeyEvent::META_DOWN;
    }
    
    void GLFW_App::__mouseMove(int x,int y)
    {
        if(m_displayTweakBar)
            TwEventMousePosGLFW(x,y);
        uint32_t buttonModifiers, keyModifiers, bothMods;
        getModifiers(buttonModifiers, keyModifiers);
        bothMods = buttonModifiers | keyModifiers;
        MouseEvent e(bothMods, x, y, bothMods, 0);
        
        if(buttonModifiers)
            mouseDrag(e);
        else
            mouseMove(e);
    }
    
    void GLFW_App::__mouseButton(int button, int action)
    {
        if(m_displayTweakBar)
            TwEventMouseButtonGLFW(button, action);
        
        uint32_t initiator, keyModifiers, bothMods;
        getModifiers(initiator, keyModifiers);
        bothMods = initiator | keyModifiers;
        
        int posX, posY;
        glfwGetMousePos(&posX, &posY);
        
        MouseEvent e(initiator, posX, posY, bothMods, 0);
        
        if (action == GLFW_PRESS)
            mousePress(e);
        else
            mouseRelease(e);
    }
    
    void GLFW_App::__mouseWheel(int pos)
    {
        if(m_displayTweakBar)
            TwEventMouseWheelGLFW(pos);
        
        int posX, posY;
        glfwGetMousePos(&posX, &posY);
        uint32_t buttonMod, keyModifiers = 0;
        getModifiers(buttonMod, keyModifiers);
        MouseEvent e(0, posX, posY, keyModifiers, pos - m_lastWheelPos);
        m_lastWheelPos = pos;
        if(running()) mouseWheel(e);
    }
    
    void GLFW_App::__keyFunc(int key, int action)
    {
        if(m_displayTweakBar)
            TwEventKeyGLFW(key, action);
            
        uint32_t buttonMod, keyMod;
        getModifiers(buttonMod, keyMod);
        
        KeyEvent e(key, key, keyMod);
        
        if(action == GLFW_PRESS)
            keyPress(e);
        else
            keyRelease(e);
    }
    
    void GLFW_App::__charFunc(int key, int action)
    {
        if(m_displayTweakBar)
            TwEventCharGLFW(key, action);

        if(key == GLFW_KEY_SPACE)
            return;
            
        uint32_t buttonMod, keyMod;
        getModifiers(buttonMod, keyMod);
        
        KeyEvent e(key, key, keyMod);
        
        if(action == GLFW_PRESS)
            keyPress(e);
        else
            keyRelease(e);
    }
    
/****************************  TweakBar + Properties **************************/
    
    void GLFW_App::addPropertyToTweakBar(const Property::Ptr propPtr,
                                    const string &group,
                                    TwBar *theBar)
    {
        if(!theBar)
        {   if(m_tweakBarList.empty()) return;
            theBar = m_tweakBarList.front();
        }
        m_tweakProperties[theBar] = propPtr;
        
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
        list<Property::Ptr>::const_iterator propIt = theProps.begin();
        
        for (; propIt != theProps.end(); propIt++) 
        {   
            addPropertyToTweakBar(*propIt, group, theBar);
        }
    }
    
    void GLFW_App::setBarPosition(const glm::ivec2 &thePos, TwBar *theBar)
    {
        if(!theBar)
        {   if(m_tweakBarList.empty()) return;
            theBar = m_tweakBarList.front();
        }
        std::stringstream ss;
        ss << TwGetBarName(theBar) << " position='" <<thePos.x
        <<" " << thePos.y <<"'";
        TwDefine(ss.str().c_str());
    }
    
    void GLFW_App::setBarSize(const glm::ivec2 &theSize, TwBar *theBar)
    {
        if(!theBar)
        {   if(m_tweakBarList.empty()) return;
            theBar = m_tweakBarList.front();
        }
        std::stringstream ss;
        ss << TwGetBarName(theBar) << " size='" <<theSize.x
        <<" " << theSize.y <<"'";
        TwDefine(ss.str().c_str());
    }
    
    void GLFW_App::setBarColor(const glm::vec4 &theColor, TwBar *theBar)
    {
        if(!theBar)
        {   if(m_tweakBarList.empty()) return;
            theBar = m_tweakBarList.front();
        }
        
        std::stringstream ss;
        glm::ivec4 color(theColor * 255);
        
        ss << TwGetBarName(theBar) << " color='" <<color.r
        <<" " << color.g << " " << color.b <<"' alpha="<<color.a;
        TwDefine(ss.str().c_str());
    }
    
    void GLFW_App::setBarTitle(const std::string &theTitle, TwBar *theBar)
    {
        if(!theBar)
        {   if(m_tweakBarList.empty()) return;
            theBar = m_tweakBarList.front();
        }
        std::stringstream ss;
        ss << TwGetBarName(theBar) << " label='" << theTitle <<"'";
        TwDefine(ss.str().c_str());
    }
}
#endif
