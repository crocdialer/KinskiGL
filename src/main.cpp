#include "KinskiGL.h"
#include "TextureIO.h"


using namespace std;

int main(int argc, char *argv[])
{
//#ifdef __APPLE__
//	string path(argv[0]);
//	path = path.substr(0,path.find_last_of("/")+1);
//	printf("executable path: %s\n",path.c_str());
//	
//	chdir( (path + "../Resources/").c_str() );
//    
//    char buf[512];
//	getcwd(buf, 512);
//	printf("current dir: %s\n",buf);
//    
//#endif

    int running = GL_TRUE;
    
    // Initialize GLFW
    if( !glfwInit() )
    {
        exit( EXIT_FAILURE );
    }
    // request an OpenGl 3.2 Context
    glfwOpenWindowHint( GLFW_VERSION_MAJOR, 3 );    
    glfwOpenWindowHint( GLFW_VERSION_MINOR, 2 );

//    glfwOpenWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
//    glfwOpenWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Open an OpenGL window
    if( !glfwOpenWindow( 300,300, 0, 0, 0, 0, 32, 0, GLFW_WINDOW ) )
    {
        glfwTerminate();
        exit( EXIT_FAILURE );
    }
    
    // version
    printf("OpenGL: %s\n", glGetString(GL_VERSION));
    printf("GLSL: %s\n",glGetString(GL_SHADING_LANGUAGE_VERSION));
    
    glClearColor(1, 0, 0, 1);
    
    
    // Main loop
    while( running )
    {
        // OpenGL rendering goes here...
        
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Swap front and back rendering buffers
        glfwSwapBuffers();
        // Check if ESC key was pressed or window was closed
        running = !glfwGetKey( GLFW_KEY_ESC ) &&
        glfwGetWindowParam( GLFW_OPENED );
    }
    
    // Close window and terminate GLFW
    glfwTerminate();

    return EXIT_SUCCESS;
}
