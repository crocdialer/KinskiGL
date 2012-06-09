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
        s_instance = shared_from_this();
        
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
        
        // version
        printf("OpenGL: %s\n", glGetString(GL_VERSION));
        printf("GLSL: %s\n",glGetString(GL_SHADING_LANGUAGE_VERSION));
        
        // AntTweakbar
        TwInit(TW_OPENGL_CORE, NULL);
        TwWindowSize(m_windowSize[0], m_windowSize[1]);
        
        m_tweakBar = TwNewBar("KinskiGL");
        
        // directly redirect GLFW events to AntTweakBar
        glfwSetMouseButtonCallback((GLFWmousebuttonfun)TwEventMouseButtonGLFW);
        glfwSetMousePosCallback((GLFWmouseposfun)TwEventMousePosGLFW);
        glfwSetMouseWheelCallback((GLFWmousewheelfun)TwEventMouseWheelGLFW);
        glfwSetKeyCallback((GLFWkeyfun)TwEventKeyGLFW);
        glfwSetCharCallback((GLFWcharfun)TwEventCharGLFW);
        
        // send window size events to AntTweakBar
        glfwSetWindowSizeCallback(&s_resize);
        
        // call user defined setup callback
        setup();
    }
    
    void App::resize(const int w,const int h)
    {
        m_windowSize = glm::ivec2(w, h);
        
        glViewport(0, 0, w, h);
        TwWindowSize(w, h);
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
            if(m_displayTweakBar) TwDraw();
            
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
    
/****************************  TweakBar + Properties **************************/ 
    
    void App::addPropertyToTweakBar(const Property::Ptr propPtr,
                               const string &group)
    {
        m_tweakProperties.push_back(propPtr);
        try {
            AntTweakBarConnector::connect(m_tweakBar, propPtr, group);
        } catch (AntTweakBarConnector::PropertyUnsupportedException &e) {
            cout<<e.getMessage()<<"\n";
        }
    }
    
    void App::addPropertyListToTweakBar(const list<Property::Ptr> &theProps,
                                     const string &group)
    {
        list<Property::Ptr>::const_iterator propIt = theProps.begin();
        
        for (; propIt != theProps.end(); propIt++) 
        {   
            addPropertyToTweakBar(*propIt, group);
        }
    }
    
    void App::loadPropertiesInShader(gl::Shader theShader,
                                     const list<Property::Ptr> &theProps)
    {
        list<Property::Ptr>::const_iterator propIt = theProps.begin();
        
        for (; propIt != theProps.end(); propIt++) 
        {   
            const Property::Ptr &aProp = *propIt;
            
            if (aProp->isOfType<int>()) 
            {
                theShader.uniform(aProp->getName(), aProp->getValue<int>());
            }
            else if (aProp->isOfType<float>()) 
            {
                theShader.uniform(aProp->getName(), aProp->getValue<float>());
            }
            else if (aProp->isOfType<glm::vec3>()) 
            {
                theShader.uniform(aProp->getName(), aProp->getValue<glm::vec3>());
            }
            else if (aProp->isOfType<glm::vec4>()) 
            {
                theShader.uniform(aProp->getName(), aProp->getValue<glm::vec4>());
            }
            else if (aProp->isOfType<glm::mat3>()) 
            {
                theShader.uniform(aProp->getName(), aProp->getValue<glm::mat3>());
            }
            else if (aProp->isOfType<glm::mat4>()) 
            {
                theShader.uniform(aProp->getName(), aProp->getValue<glm::mat4>());
            }

        }
    }
}