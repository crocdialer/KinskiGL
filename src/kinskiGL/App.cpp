#include "App.h"

namespace kinski
{
    
    App::App(const int width, const int height):m_running(GL_FALSE),
    m_displayTweakBar(true)
    {
        // Initialize GLFW
        if( !glfwInit() )
        {
            throw std::exception();
        }
        
        // request an OpenGl 3.2 Context
        glfwOpenWindowHint( GLFW_OPENGL_VERSION_MAJOR, 3 );    
        glfwOpenWindowHint( GLFW_OPENGL_VERSION_MINOR, 2 );
        glfwOpenWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        //glfwOpenWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        // Open an OpenGL window
        if( !glfwOpenWindow( width, height, 0, 0, 0, 0, 32, 0, GLFW_WINDOW ) )
        {
            glfwTerminate();
            throw std::exception();
        }
        
        // version
        printf("OpenGL: %s\n", glGetString(GL_VERSION));
        printf("GLSL: %s\n",glGetString(GL_SHADING_LANGUAGE_VERSION));
        
        // AntTweakbar
        TwInit(TW_OPENGL_CORE, NULL);
        TwWindowSize(width, height);
        
        m_tweakBar = TwNewBar("papa Jango:");
        TwAddVarRW(m_tweakBar, "testFloat", TW_TYPE_FLOAT, &m_testFloat, "");
        
        // directly redirect GLFW events to AntTweakBar
        glfwSetMouseButtonCallback((GLFWmousebuttonfun)TwEventMouseButtonGLFW);
        glfwSetMousePosCallback((GLFWmouseposfun)TwEventMousePosGLFW);
        glfwSetMouseWheelCallback((GLFWmousewheelfun)TwEventMouseWheelGLFW);
        glfwSetKeyCallback((GLFWkeyfun)TwEventKeyGLFW);
        glfwSetCharCallback((GLFWcharfun)TwEventCharGLFW);
        
        // send window size events to AntTweakBar
        glfwSetWindowSizeCallback(&resize);
        
    }
    
    App::~App()
    {
        TwTerminate();
        
        // Close window and terminate GLFW
        glfwTerminate();
    }

    void App::resize(int w, int h)
    {
        TwWindowSize(w, h);
    }

    
    int App::run()
    {
        
        m_running = GL_TRUE;
        
        // Main loop
        while( m_running )
        {
            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            update(0);
            draw();
            
            // tweakbar
            if(m_displayTweakBar) TwDraw();
            
            // Swap front and back rendering buffers
            glfwSwapBuffers();
            
            // Check if ESC key was pressed or window was closed
            m_running = !glfwGetKey( GLFW_KEY_ESC ) &&
            glfwGetWindowParam( GLFW_OPENED );
        }

        return EXIT_SUCCESS;
    }

}