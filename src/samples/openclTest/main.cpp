#include "kinskiGL/App.h"

#include "kinskiGL/Texture.h"
#include "kinskiGL/Shader.h"

#include "Data.h"

#include "kinskiGL/TextureIO.h"
#include "kinskiCV/CVThread.h"

#include <fstream>

//#include <OpenCL/opencl.h>
#define __CL_ENABLE_EXCEPTIONS
#include "cl.hpp"


using namespace std;
using namespace kinski;
using namespace glm;

class OpenCLTest : public App
{
private:
    
    gl::Texture m_texture;
    gl::Shader m_shader;
    
    GLuint m_canvasBuffer;
    GLuint m_canvasArray;
    
    _Property<bool>::Ptr m_activator;
    
    CVThread::Ptr m_cvThread;
    
    void buildCanvasVBO()
    {
        //GL_T2F_V3F
        const GLfloat array[] ={0.0,0.0,0.0,0.0,0.0,
                                1.0,0.0,1.0,0.0,0.0,
                                1.0,1.0,1.0,1.0,0.0,
                                0.0,1.0,0.0,1.0,0.0};
        
        // create VAO to record all VBO calls
        glGenVertexArrays(1, &m_canvasArray);
        glBindVertexArray(m_canvasArray);
        
        glGenBuffers(1, &m_canvasBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_canvasBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(array), array, GL_STATIC_DRAW);
        
        GLsizei stride = 5 * sizeof(GLfloat);
        
        GLuint positionAttribLocation = m_shader.getAttribLocation("a_position");
        glEnableVertexAttribArray(positionAttribLocation);
        glVertexAttribPointer(positionAttribLocation, 3, GL_FLOAT, GL_FALSE,
                              stride, BUFFER_OFFSET(2 * sizeof(GLfloat)));
        
        GLuint texCoordAttribLocation = m_shader.getAttribLocation("a_texCoord");
        glEnableVertexAttribArray(texCoordAttribLocation);
        glVertexAttribPointer(texCoordAttribLocation, 2, GL_FLOAT, GL_FALSE,
                              stride, BUFFER_OFFSET(0));
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        glBindVertexArray(0);
        
    }
    
    void drawTexture(gl::Texture theTexture,
                     const vec2 &theTl = vec2(0),
                     const vec2 &theSize = vec2(0))
    {
        vec2 sz = theSize == vec2(0) ? theTexture.getSize() : theSize;
        vec2 tl = theTl == vec2(0) ? vec2(0, getHeight()) : theTl;
        drawTexture(theTexture, tl[0], tl[1], (tl+sz)[0], tl[1]-sz[1]);
    }
    
    
    void drawTexture(const gl::Texture &theTexture, float x0, float y0, float x1, float y1)
    {
        // Texture and Shader bound for this scope
        gl::scoped_bind<gl::Texture> texBind(theTexture);
        
        // orthographic projection with a [0,1] coordinate space
        static mat4 projectionMatrix = ortho(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
        
        float scaleX = (x1 - x0) / getWidth();
        float scaleY = (y0 - y1) / getHeight();
        
        mat4 modelViewMatrix = glm::scale(mat4(), vec3(scaleX, scaleY, 1));
        modelViewMatrix[3] = vec4(x0 / getWidth(), y1 / getHeight() , 0, 1);
        
        m_shader.uniform("u_textureMap", theTexture.getBoundTextureUnit());
        m_shader.uniform("u_textureMatrix", theTexture.getTextureMatrix());
        
        m_shader.uniform("u_modelViewProjectionMatrix", 
                         projectionMatrix * modelViewMatrix);
        
        glBindVertexArray(m_canvasArray);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
    
public:
    
    void setup()
    {  
        glEnable(GL_TEXTURE_2D);
        glClearColor(0, 0, 0, 1);
        
        try
        {
            m_shader.loadFromData(g_vertShaderSrc, g_fragShaderSrc);
        }
        catch (std::exception &e)
        {
            fprintf(stderr, "%s\n",e.what());
        }
        
        buildCanvasVBO();
        
        m_activator = _Property<bool>::create("processing", true);
        
        // add component-props to tweakbar
        addPropertyToTweakBar(m_activator);
        
        // CV stuff 
        m_cvThread = CVThread::Ptr(new CVThread());
        CVSourceNode::Ptr sourceNode(new CVCaptureNode);
        CVProcessNode::Ptr procNode;
        
        m_cvThread->setSourceNode(sourceNode);
        m_cvThread->start();
        
        if(procNode) addPropertyListToTweakBar(procNode->getPropertyList());
        cout<<"CVThread source: \n"<<m_cvThread->getSourceInfo()<<"\n";
        
        // OpenCL
        
        try
        {
            // Get available platforms
            vector<cl::Platform> platforms;
            cl::Platform::get(&platforms);
            
            // Select the default platform and create a context using this platform and the GPU
            cl_context_properties cps[3] = {
                CL_CONTEXT_PLATFORM,
                (cl_context_properties)(platforms[0])(),
                0
            };
            cl::Context context( CL_DEVICE_TYPE_GPU, cps);
            
            // Get a list of devices on this platform
            vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
            
            // Create a command queue and use the first device
            cl::CommandQueue queue = cl::CommandQueue(context, devices[0]);
            
            // Read source file
            std::ifstream sourceFile("vector_add_kernel.cl");
            std::string sourceCode(std::istreambuf_iterator<char>(sourceFile),
                                   (std::istreambuf_iterator<char>()));
            
            cl::Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length()+1));
            
            // Make program of the source code in the context
            cl::Program program = cl::Program(context, source);
            
            // Build program for these specific devices
            program.build(devices);
            
            // Make kernel
            cl::Kernel kernel(program, "vector_add");
            
            // Create memory buffers
            const int LIST_SIZE = 100;
            cl::Buffer bufferA = cl::Buffer(context, CL_MEM_READ_ONLY, LIST_SIZE * sizeof(int));
            cl::Buffer bufferB = cl::Buffer(context, CL_MEM_READ_ONLY, LIST_SIZE * sizeof(int));
            cl::Buffer bufferC = cl::Buffer(context, CL_MEM_WRITE_ONLY, LIST_SIZE * sizeof(int));
            
            // Copy lists A and B to the memory buffers
//            queue.enqueueWriteBuffer(bufferA, CL_TRUE, 0, LIST_SIZE * sizeof(int), A);
//            queue.enqueueWriteBuffer(bufferB, CL_TRUE, 0, LIST_SIZE * sizeof(int), B);
            
            // Set arguments to kernel
            kernel.setArg(0, bufferA);
            kernel.setArg(1, bufferB);
            kernel.setArg(2, bufferC);
            
            // Run the kernel on specific ND range
            cl::NDRange global(LIST_SIZE);
            cl::NDRange local(1);
            queue.enqueueNDRangeKernel(kernel, cl::NullRange, global, local);
            
            // Read buffer C into a local list
            int *C = new int[LIST_SIZE];
            queue.enqueueReadBuffer(bufferC, CL_TRUE, 0, LIST_SIZE * sizeof(int), C);
            
//            for(int i = 0; i < LIST_SIZE; i ++)
//                std::cout << A[i] << " + " << B[i] << " = " << C[i] << std::endl; 
        }
        catch(cl::Error &error)
        {
            std::cout << error.what() << "(" << error.err() << ")" << std::endl;
        }
    }
    
    void tearDown()
    {
        m_cvThread->stop();
        
        printf("ciao openclTest\n");
    }
    
    void update(const float timeDelta)
    {
        cv::Mat camFrame;
        
        if(m_cvThread->getImage(camFrame))
        {
            TextureIO::updateTexture(m_texture, camFrame);
        }
        
        // trigger processing
        m_cvThread->setProcessing(m_activator->val());
    }
    
    void draw()
    {
        gl::scoped_bind<gl::Shader> shaderBind(m_shader);
        drawTexture(m_texture, vec2(0, getHeight()), getWindowSize());
        
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new OpenCLTest);
    
    return theApp->run();
}
