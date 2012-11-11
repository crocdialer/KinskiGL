#include "kinskiGL/App.h"
#include "kinskiGL/TextureIO.h"
#include "kinskiGL/SerializerGL.h"
#include <fstream>

//#include <OpenCL/opencl.h>
//#pragma OPENCL EXTENSION CL_APPLE_gl_sharing : enable
//#pragma OPENCL EXTENSION CL_KHR_gl_sharing : enable

#define __CL_ENABLE_EXCEPTIONS
#include "cl.hpp"


using namespace std;
using namespace kinski;
using namespace glm;

class OpenCLTest : public App
{
private:
    
    Property_<string>::Ptr m_texturePath;
    gl::Texture m_texture;
    
    //OpenCL members
    cl::Context m_context;
    cl::Device m_device;
    cl::CommandQueue m_queue;
    cl::Program m_program;
    
    void initOpenCL()
    {
        // OpenCL
        try
        {
            // Get available platforms
            vector<cl::Platform> platforms;
            cl::Platform::get(&platforms);
            
            // context sharing is OS specific
            #if defined (__APPLE__) || defined(MACOSX)
                CGLContextObj curCGLContext = CGLGetCurrentContext();
                CGLShareGroupObj curCGLShareGroup = CGLGetShareGroup(curCGLContext);
                
                cl_context_properties properties[] =
                {
                    CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
                    (cl_context_properties)curCGLShareGroup,
                    0
                };
            #elif defined WIN32
                cl_context_properties properties[] =
                {
                    CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
                    CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
                    CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(),
                    0
                };
            #else
                cl_context_properties properties[] =
                {
                    CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
                    CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
                    CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(),
                    0
                };
            #endif
            
            m_context = cl::Context( CL_DEVICE_TYPE_GPU, properties);
            
            // Get a list of devices on this platform
            vector<cl::Device> devices = m_context.getInfo<CL_CONTEXT_DEVICES>();
            m_device = devices[0];
            
            // Create a command queue and use the first device
            m_queue = cl::CommandQueue(m_context, devices[0]);
            
            // Read source file
            std::ifstream sourceFile("vector_add_kernel.cl");
            std::string sourceCode(std::istreambuf_iterator<char>(sourceFile),
                                   (std::istreambuf_iterator<char>()));
            
            cl::Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length()+1));
            
            // Make program of the source code in the context
            m_program = cl::Program(m_context, source);
            
            // Build program for these specific devices
            m_program.build();
        }
        catch(cl::Error &error)
        {
            std::cout << error.what() << "(" << error.err() << ")" << std::endl;
        }
    }
    
    void runOpenCL()
    {
        // OpenCL
        try
        {
            // Make kernel
            cl::Kernel kernel(m_program, "vector_add");
            
            // Create memory buffers
            const int LIST_SIZE = 100;
            cl::Buffer bufferA = cl::Buffer(m_context, CL_MEM_READ_ONLY, LIST_SIZE * sizeof(int));
            cl::Buffer bufferB = cl::Buffer(m_context, CL_MEM_READ_ONLY, LIST_SIZE * sizeof(int));
            cl::Buffer bufferC = cl::Buffer(m_context, CL_MEM_WRITE_ONLY, LIST_SIZE * sizeof(int));
            
            // Copy lists A and B to the memory buffers
            int A[LIST_SIZE];
            int B[LIST_SIZE];
            for(int i = 0; i < LIST_SIZE; i++)
            {
                A[i] = i;
                B[i] = LIST_SIZE - i;
            }
            m_queue.enqueueWriteBuffer(bufferA, CL_TRUE, 0, LIST_SIZE * sizeof(int), A);
            m_queue.enqueueWriteBuffer(bufferB, CL_TRUE, 0, LIST_SIZE * sizeof(int), B);
            
            // Set arguments to kernel
            kernel.setArg(0, bufferA);
            kernel.setArg(1, bufferB);
            kernel.setArg(2, bufferC);
            
            // Run the kernel on specific ND range
            cl::NDRange global(LIST_SIZE);
            //cl::NDRange local(1);
            m_queue.enqueueNDRangeKernel(kernel, cl::NullRange, global);
            
            // Read buffer C into a local list
            int C [LIST_SIZE];
            m_queue.enqueueReadBuffer(bufferC, CL_TRUE, 0, LIST_SIZE * sizeof(int), C);
            
            for(int i = 0; i < LIST_SIZE; i ++)
                std::cout << A[i] << " + " << B[i] << " = " << C[i] << std::endl;
        }
        catch(cl::Error &error)
        {
            std::cout << error.what() << "(" << error.err() << ")" << std::endl;
        }
    }
    
public:
    
    void setup()
    {
        m_texturePath = Property_<string>::create("Texture path", "");
        registerProperty(m_texturePath);

        addPropertyListToTweakBar(getPropertyList());
        observeProperties();
        
        // load state from config file
        try
        {
            Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
        }catch(FileNotFoundException &e)
        {
            printf("%s\n", e.what());
        }
        
        initOpenCL();
        runOpenCL();
    }
    
    void tearDown()
    {
        printf("ciao openclTest\n");
    }
    
    void update(const float timeDelta)
    {
    
    }
    
    void draw()
    {
        gl::drawTexture(m_texture, getWindowSize());
    }
    
    void updateProperty(const Property::Ptr &theProperty)
    {
        // one of our porperties was changed
        if(theProperty == m_texturePath)
        {
            try
            {
                m_texture = gl::TextureIO::loadTexture(m_texturePath->val());
            } catch (gl::TextureIO::TextureNotFoundException &e)
            {
                cout<<"WARNING: "<< e.what() << endl;
                
                m_texturePath->removeObserver(shared_from_this());
                m_texturePath->val("- not found -");
                m_texturePath->addObserver(shared_from_this());
            }
        }
    }
    
    void keyPress(const KeyEvent &e)
    {
        switch (e.getChar())
        {
            case KeyEvent::KEY_s:
                Serializer::saveComponentState(shared_from_this(), "config.json", PropertyIO_GL());
                break;
                
            case KeyEvent::KEY_r:
                Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
                break;
                
            default:
                break;
        }
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new OpenCLTest);
    
    return theApp->run();
}
