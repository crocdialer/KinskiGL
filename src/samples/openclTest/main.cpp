#include "kinskiApp/GLFW_App.h"
#include "kinskiApp/TextureIO.h"
#include "kinskiGL/Material.h"
#include "kinskiGL/Buffer.h"
#include "kinskiGL/Camera.h"

#include "kinskiGL/SerializerGL.h"
#include <fstream>

#define __CL_ENABLE_EXCEPTIONS
#include "cl.hpp"

#define RAND(min, max) min + (max - min) * (rand() / (float) RAND_MAX)

using namespace std;
using namespace kinski;
using namespace glm;

class OpenCLTest : public GLFW_App
{
private:
    
    RangedProperty<float>::Ptr m_distance;
    Property_<glm::mat3>::Ptr m_rotation;
    
    RangedProperty<float>::Ptr m_rotationSpeed;
    
    Property_<string>::Ptr m_texturePath;
    gl::Texture m_texture;
    
    gl::Material::Ptr m_pointMaterial;
    gl::PerspectiveCamera::Ptr m_camera;
    
    // mouse rotation control
    vec2 m_clickPos;
    mat4 m_lastTransform;
    float m_lastDistance;
    
    
    gl::Buffer m_vboPos, m_vboCol;
    
    //OpenCL standard stuff
    cl::Context m_context;
    cl::Device m_device;
    cl::CommandQueue m_queue;
    cl::Program m_program;
    
    // particle system related
    GLsizei m_numParticles;
    cl::Kernel m_particleKernel;
    cl::Buffer m_velocities, m_positionGen, m_velocityGen;
    cl::BufferGL m_positions;
    
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
            std::ifstream sourceFile("kernels.cl");
            std::string sourceCode(std::istreambuf_iterator<char>(sourceFile),
                                   (std::istreambuf_iterator<char>()));
            
            cl::Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length()+1));
            
            // Make program of the source code in the context
            m_program = cl::Program(m_context, source);
            
            // Build program for these specific devices
            m_program.build();
            
            m_particleKernel = cl::Kernel(m_program, "updateParticles");
        }
        catch(cl::Error &error)
        {
            LOG_ERROR << error.what() << "(" << error.err() << ")";
        }
    }
    
    void updateParticles(float timeDelta)
    {
        try
        {
            vector<cl::Memory> glBuffers;
            glBuffers.push_back(m_positions);
            
            //this will update our system by calculating new velocity and updating the positions of our particles
            //Make sure OpenGL is done using our VBOs
            glFinish();
            
            // map OpenGL buffer object for writing from OpenCL
            // this passes in the vector of VBO buffer objects (position and color)
            m_queue.enqueueAcquireGLObjects(&glBuffers);
            
            //m_queue.finish();
            
            m_particleKernel.setArg(4, timeDelta); //pass in the timestep
            
            //execute the kernel
            m_queue.enqueueNDRangeKernel(m_particleKernel, cl::NullRange, cl::NDRange(m_numParticles),
                                         cl::NullRange);
            
            //m_queue.finish();
            
            //Release the VBOs so OpenGL can play with them
            m_queue.enqueueReleaseGLObjects(&glBuffers, NULL);

            m_queue.finish();
        }
        catch(cl::Error &error)
        {
            LOG_ERROR << error.what() << "(" << error.err() << ")";
        }
    }
    
public:
    
    void setup()
    {
        m_distance = RangedProperty<float>::create("view distance", 25, -500, 500);
        registerProperty(m_distance);
        
        m_rotation = Property_<glm::mat3>::create("Geometry Rotation", glm::mat3());
        registerProperty(m_rotation);
        
        m_rotationSpeed = RangedProperty<float>::create("Rotation Speed", 0, -100, 100);
        registerProperty(m_rotationSpeed);
        
        m_texturePath = Property_<string>::create("Texture path", "smoketex.png");
        registerProperty(m_texturePath);
        
        addPropertyListToTweakBar(getPropertyList());
        observeProperties();
        
        m_pointMaterial = gl::Material::Ptr(new gl::Material);
        m_pointMaterial->addTexture(gl::TextureIO::loadTexture("smoketex.png"));
        m_pointMaterial->setPointSize(9.f);
        m_pointMaterial->setDiffuse(vec4(.9, .7, 0, 1.f));
        m_pointMaterial->setBlending();
        
        m_camera = gl::PerspectiveCamera::Ptr(new gl::PerspectiveCamera);
        
        m_camera->setPosition(vec3(0, 50, 100));
        m_camera->setLookAt(vec3(0));
        
        // load state from config file
        try
        {
            Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
        }catch(FileNotFoundException &e)
        {
            printf("%s\n", e.what());
        }
        
        initOpenCL();
        
        m_numParticles = 50000;
        GLsizei numBytes = m_numParticles * sizeof(vec4);
        
        m_vboPos = gl::Buffer(GL_ARRAY_BUFFER, GL_STREAM_DRAW);
        m_vboPos.setData(NULL, numBytes);
        
        try
        {
            // shared position buffer for OpenGL / OpenCL
            m_positions = cl::BufferGL(m_context, CL_MEM_READ_WRITE, m_vboPos.id());
            
            //create the OpenCL only arrays
            m_velocities = cl::Buffer( m_context, CL_MEM_WRITE_ONLY, numBytes );
            m_positionGen = cl::Buffer( m_context, CL_MEM_WRITE_ONLY, numBytes );
            m_velocityGen = cl::Buffer( m_context, CL_MEM_WRITE_ONLY, numBytes );
            
            srand(clock());
            
            vector<vec4> posGen, velGen;
            for (int i = 0; i < m_numParticles; i++)
            {
                posGen.push_back( vec4(glm::ballRand(20.0f), 1.f) );
                
                vec2 tmp = glm::linearRand(vec2(-10), vec2(10));
                
                float life = 2 + 3 * (rand() / (float) RAND_MAX);
                float yVel = RAND(5, 15);
                
                velGen.push_back(vec4(tmp.x, yVel, tmp.y, life)); 
            }
            
            m_vboPos.setData(posGen);
            
            
            m_queue.enqueueWriteBuffer(m_velocities, CL_TRUE, 0, numBytes,
                                       &velGen[0]);
            m_queue.enqueueWriteBuffer(m_positionGen, CL_TRUE, 0, numBytes,
                                       &posGen[0]);
            m_queue.enqueueWriteBuffer(m_velocityGen, CL_TRUE, 0, numBytes,
                                       &velGen[0]);
            
            m_particleKernel.setArg(0, m_positions);
            //m_particleKernel.setArg(1, m_positions);//colors
            
            m_particleKernel.setArg(1, m_velocities);
            m_particleKernel.setArg(2, m_positionGen);
            m_particleKernel.setArg(3, m_velocityGen);
        }
        catch(cl::Error &error)
        {
            LOG_ERROR << error.what() << "(" << error.err() << ")";
        }
    }
    
    void tearDown()
    {
        printf("ciao openclTest\n");
    }
    
    void update(const float timeDelta)
    {
        updateParticles(timeDelta);
        
        *m_rotation = mat3( glm::rotate(mat4(m_rotation->val()),
                                        m_rotationSpeed->val() * timeDelta,
                                        vec3(0, 1, .5)));
    }
    
    void draw()
    {
        gl::loadMatrix(gl::PROJECTION_MATRIX, m_camera->getProjectionMatrix());
        gl::loadMatrix(gl::MODEL_VIEW_MATRIX, m_camera->getViewMatrix());
        
        gl::drawGrid(200, 200);
        
        //gl::drawTexture(m_texture, getWindowSize());
        
        gl::drawPoints(m_vboPos.id(), m_numParticles, m_pointMaterial, sizeof(vec4));
    }
    
    void updateProperty(const Property::ConstPtr &theProperty)
    {
        // one of our porperties was changed
        if(theProperty == m_texturePath)
        {
            try
            {
                m_texture = gl::createTextureFromFile(m_texturePath->val());
            } catch (kinski::Exception &e)
            {
                LOG_WARNING << e.what();
                
                m_texturePath->removeObserver(shared_from_this());
                m_texturePath->val("- not found -");
                m_texturePath->addObserver(shared_from_this());
            }
        }
        else if(theProperty == m_distance ||
                 theProperty == m_rotation)
        {
            m_camera->setPosition( m_rotation->val() * glm::vec3(0, 0, m_distance->val()) );
            m_camera->setLookAt(glm::vec3(0));
        }
    }
    
    void resize(int w, int h)
    {
        m_camera->setAspectRatio(getAspectRatio());
    }
    
    void mousePress(const MouseEvent &e)
    {
        m_clickPos = vec2(e.getX(), e.getY());
        m_lastTransform = mat4(m_rotation->val());
        m_lastDistance = m_distance->val();
    }
    
    void mouseDrag(const MouseEvent &e)
    {
        vec2 mouseDiff = vec2(e.getX(), e.getY()) - m_clickPos;
        if(e.isLeft() && e.isAltDown())
        {
            mat4 mouseRotate = glm::rotate(m_lastTransform, mouseDiff.x, vec3(0, 1, 0));
            mouseRotate = glm::rotate(mouseRotate, mouseDiff.y, vec3(1, 0, 0));
            *m_rotation = mat3(mouseRotate);
        }
        else if(e.isRight())
        {
            *m_distance = m_lastDistance + 0.3f * mouseDiff.y;
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
