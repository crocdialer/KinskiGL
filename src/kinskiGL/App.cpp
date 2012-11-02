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
    m_windowSize(glm::ivec2(width, height)),
    m_fullscreen(false), m_displayTweakBar(true)
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
        
        glfwSwapInterval(1);
        
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
        
        glfwSetKeyCallback((GLFWkeyfun)TwEventKeyGLFW);
        glfwSetCharCallback((GLFWcharfun)TwEventCharGLFW);
        
        // send window size events to AntTweakBar
        glfwSetWindowSizeCallback(&s_resize);
        
        // call user defined setup callback
        setup();
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
        TwWindowSize(w, h);
        
        // user hook
        resize(w, h);
    }
    
    void App::__mouseMove(int x,int y)
    {
        if(m_displayTweakBar)
            TwEventMousePosGLFW(x,y);
        
        uint32_t buttonModifier = 0;
        if( glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT) )
            buttonModifier |= MouseEvent::LEFT_DOWN;
        if( glfwGetMouseButton(GLFW_MOUSE_BUTTON_MIDDLE) )
            buttonModifier |= MouseEvent::MIDDLE_DOWN;
        if( glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT) )
            buttonModifier |= MouseEvent::RIGHT_DOWN;
            
        MouseEvent e(0, x, y, buttonModifier, 0);
        
        if(buttonModifier)
            mouseDrag(e);
        else
            mouseMove(e);
    }
    
    void App::__mouseButton(int button, int action)
    {
        if(m_displayTweakBar)
            TwEventMouseButtonGLFW(button, action);
        
        uint32_t initiator = 0;
        switch (button)
        {
            case GLFW_MOUSE_BUTTON_LEFT:
                initiator = MouseEvent::LEFT_DOWN;
                break;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                initiator = MouseEvent::MIDDLE_DOWN;
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                initiator = MouseEvent::RIGHT_DOWN;
                break;
                
            default:
                break;
        }
        
        uint32_t keyModifiers = initiator;
        if( glfwGetKey(GLFW_KEY_LCTRL) || glfwGetKey(GLFW_KEY_RCTRL))
            keyModifiers |= MouseEvent::CTRL_DOWN;
        if( glfwGetKey(GLFW_KEY_LSHIFT) || glfwGetKey(GLFW_KEY_RSHIFT))
            keyModifiers |= MouseEvent::SHIFT_DOWN;
        if( glfwGetKey(GLFW_KEY_LALT) || glfwGetKey(GLFW_KEY_RALT))
            keyModifiers |= MouseEvent::ALT_DOWN;
        
        int posX, posY;
        glfwGetMousePos(&posX, &posY);
        
        MouseEvent e(initiator, posX, posY, keyModifiers, 0);
        
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
        
        uint32_t keyModifiers = 0;
        if( glfwGetKey(GLFW_KEY_LCTRL) || glfwGetKey(GLFW_KEY_RCTRL))
            keyModifiers |= MouseEvent::CTRL_DOWN;
        if( glfwGetKey(GLFW_KEY_LSHIFT) || glfwGetKey(GLFW_KEY_RSHIFT))
            keyModifiers |= MouseEvent::SHIFT_DOWN;
        if( glfwGetKey(GLFW_KEY_LALT) || glfwGetKey(GLFW_KEY_RALT))
            keyModifiers |= MouseEvent::ALT_DOWN;
        
        MouseEvent e(0, posX, posY, keyModifiers, pos);
        
        mouseWheel(e);
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
}