#include "App.h"
#include "AntTweakBarConnector.h"
#include <iostream>

using namespace std;

namespace kinski
{
    App::WeakPtr App::s_instance;
    
    App::App(const int width, const int height):
    m_running(GL_FALSE),
    m_lastTimeStamp(0.0), m_framesDrawn(0),
    m_displayTweakBar(true),
    m_windowSize(glm::ivec2(width, height)),
    m_fullscreen(false),
    m_cursorVisible(true)
    {
        
    }
    
    App::~App()
    {
        TwTerminate();
        
        // Close window and terminate GLFW
        glfwTerminate();
    }
    
    App::Ptr App::getInstance()
    {
        return s_instance.lock();
    }
    
    void App::init()
    {
        s_instance = dynamic_pointer_cast<App>(shared_from_this());
        
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
        if( !glfwOpenWindow( m_windowSize[0], m_windowSize[1], 0, 0, 0, 0, 24, 0,
                            m_fullscreen ? GLFW_FULLSCREEN : GLFW_WINDOW ) )
        {
            glfwTerminate();
            throw exception();
        }
        
        // show mouse cursor in fullscreen ?
        if(m_fullscreen && m_cursorVisible) glfwEnable(GLFW_MOUSE_CURSOR);
        
        glfwSwapInterval(1);
        glClearColor(0, 0, 0, 1);
        
        // version
        printf("OpenGL: %s\n", glGetString(GL_VERSION));
        printf("GLSL: %s\n",glGetString(GL_SHADING_LANGUAGE_VERSION));
        
        // AntTweakbar
        TwInit(TW_OPENGL_CORE, NULL);
        TwWindowSize(m_windowSize[0], m_windowSize[1]);
        
        m_tweakBarList.push_back(TwNewBar("KinskiGL"));
        
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
    
    void App::setWindowSize(const glm::ivec2 size)
    {
        if(glm::vec2(size) != m_windowSize)
            glfwSetWindowSize(size[0], size[1]);
    }
    
    int App::run()
    {
        init();
        
        m_running = GL_TRUE;
        
        double timeStamp = 0.0;
        
        // Main loop
        while( m_running )
        {
            glDepthMask(GL_TRUE);
            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            // update application time
            timeStamp = glfwGetTime();
            
            // call update callback
            update(timeStamp - m_lastTimeStamp);
            
            m_lastTimeStamp = timeStamp;
            
            // call draw callback
            draw();
            
            // draw tweakbar
            if(m_displayTweakBar)
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                TwDraw();
            }
            
            // Swap front and back rendering buffers
            glfwSwapBuffers();
            
            m_framesDrawn++;
            
            // Check if ESC key was pressed or window was closed
            m_running = !glfwGetKey( GLFW_KEY_ESC ) &&
            glfwGetWindowParam( GLFW_OPENED );
        }
        
        // manage tearDown, save stuff etc.
        tearDown();
        
        return EXIT_SUCCESS;
    }
    
    double App::getApplicationTime()
    {
        return glfwGetTime();
    }
/****************************  Application Events (internal) **************************/
    
    void App::__resize(int w,int h)
    {
        m_windowSize = glm::ivec2(w, h);
        
        
        glViewport(0, 0, w, h);
        gl::setWindowDimension(m_windowSize);
        
        TwWindowSize(w, h);
        
        // user hook
        if(m_running) resize(w, h);
    }
    
    void App::getModifiers(uint32_t &buttonModifiers, uint32_t &keyModifiers)
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
    
    void App::__mouseMove(int x,int y)
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
    
    void App::__mouseButton(int button, int action)
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
    
    void App::__mouseWheel(int pos)
    {
        if(m_displayTweakBar)
            TwEventMouseWheelGLFW(pos);
        
        int posX, posY;
        glfwGetMousePos(&posX, &posY);
        
        uint32_t buttonMod, keyModifiers = 0;
        getModifiers(buttonMod, keyModifiers);
        
        MouseEvent e(0, posX, posY, keyModifiers, pos);
        
        mouseWheel(e);
    }
    
    void App::__keyFunc(int key, int action)
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
    
    void App::__charFunc(int key, int action)
    {
        if(m_displayTweakBar)
            TwEventCharGLFW(key, action);
        
        uint32_t buttonMod, keyMod;
        getModifiers(buttonMod, keyMod);
        
        KeyEvent e(key, key, keyMod);
        
        if(action == GLFW_PRESS)
            keyPress(e);
        else
            keyRelease(e);
    }
    
/****************************  TweakBar + Properties **************************/
    
    void App::addPropertyToTweakBar(const Property::Ptr propPtr,
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
            cout<<e.what()<<"\n";
        }
    }
    
    void App::addPropertyListToTweakBar(const list<Property::Ptr> &theProps,
                                        const string &group,
                                        TwBar *theBar)
    {
        list<Property::Ptr>::const_iterator propIt = theProps.begin();
        
        for (; propIt != theProps.end(); propIt++) 
        {   
            addPropertyToTweakBar(*propIt, group, theBar);
        }
    }
    
    void App::setBarPosition(const glm::ivec2 &thePos, TwBar *theBar)
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
    
    void App::setBarSize(const glm::ivec2 &theSize, TwBar *theBar)
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
    
    void App::setBarColor(const glm::vec4 &theColor, TwBar *theBar)
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
    
    void App::setBarTitle(const std::string &theTitle, TwBar *theBar)
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