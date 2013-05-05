#include "kinskiApp/ViewerApp.h"
#include "kinskiGL/Fbo.h"

// OpenCL
#define __CL_ENABLE_EXCEPTIONS
#include "cl.hpp"

// OpenNI
#include "OpenNIConnector.h"

//Syphon
#include "SyphonConnector.h"

using namespace std;
using namespace kinski;
using namespace glm;

class OpenCLTest : public ViewerApp
{
private:
    
    Property_<string>::Ptr m_texturePath;
    Property_<vec4>::Ptr m_point_color;
    gl::Texture m_textures[4];
    gl::Material::Ptr m_pointMaterial;
    gl::GeometryPtr m_geom;
    gl::MeshPtr m_mesh;
    gl::Font m_font;
    
    //OpenCL standard stuff
    cl::Context m_context;
    cl::Device m_device;
    cl::CommandQueue m_queue;
    cl::Program m_program;
    
    // particle system related
    GLsizei m_numParticles;
    cl::Kernel m_particleKernel, m_imageKernel;
    cl::Buffer m_velocities, m_positionGen, m_velocityGen;
    cl::BufferGL m_positions, m_colors;
    cl::ImageGL m_cl_image;
    
    // perspective experiment
    gl::PerspectiveCamera::Ptr m_free_camera;
    gl::MeshPtr m_free_camera_mesh;
    gl::Fbo m_fbo;
    Property_<glm::vec2>::Ptr m_fbo_size;
    
    // output via Syphon
    gl::SyphonConnector m_syphon;
    Property_<bool>::Ptr m_debug_draw;
    Property_<bool>::Ptr m_use_syphon;
    Property_<std::string>::Ptr m_syphon_server_name;
    
    // OpenNI interface
    gl::OpenNIConnector::Ptr m_open_ni;
    
    void initOpenCL()
    {
        // OpenCL
        try
        {
            // Get available platforms
            vector<cl::Platform> platforms;
            cl::Platform::get(&platforms);
            LOG_INFO<<platforms.front().getInfo<CL_PLATFORM_VERSION>();
            
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
            std::string sourceCode = kinski::readFile("kernels.cl");
            
            // Make program of the source code in the context
            m_program = cl::Program(m_context, sourceCode);
            
            // Build program for these specific devices
            m_program.build();
            
            m_particleKernel = cl::Kernel(m_program, "updateParticles");
            m_imageKernel = cl::Kernel(m_program, "set_colors_from_image");
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
            glBuffers.push_back(m_colors);
            
            //this will update our system by calculating new velocity and updating the positions of our particles
            //Make sure OpenGL is done using our VBOs
            glFinish();
            
            // map OpenGL buffer object for writing from OpenCL
            // this passes in the vector of VBO buffer objects (position and color)
            m_queue.enqueueAcquireGLObjects(&glBuffers);
            
            
            m_particleKernel.setArg(5, timeDelta); //pass in the timestep
            
            //execute the kernel
            m_queue.enqueueNDRangeKernel(m_particleKernel, cl::NullRange, cl::NDRange(m_numParticles),
                                         cl::NullRange);
            
            
            //Release the VBOs so OpenGL can play with them
            m_queue.enqueueReleaseGLObjects(&glBuffers, NULL);

            m_queue.finish();
        }
        catch(cl::Error &error)
        {
            LOG_ERROR << error.what() << "(" << error.err() << ")";
        }
    }
    
    void setColors()
    {
        try
        {
            vector<cl::Memory> glBuffers;
            glBuffers.push_back(m_positions);
            glBuffers.push_back(m_colors);
            glBuffers.push_back(m_cl_image);
            
            glFinish();
            m_queue.enqueueAcquireGLObjects(&glBuffers);
            
            m_imageKernel.setArg(0, m_cl_image);
            m_imageKernel.setArg(1, m_positions);
            m_imageKernel.setArg(2, m_colors);
            
            //execute the kernel
            m_queue.enqueueNDRangeKernel(m_imageKernel, cl::NullRange, cl::NDRange(m_numParticles),
                                         cl::NullRange);
            
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
        ViewerApp::setup();
        kinski::addSearchPath("/Library/Fonts");
        kinski::addSearchPath("~/Desktop");
        kinski::addSearchPath("~/Pictures");
        m_font.load("Courier New Bold.ttf", 30);
        
        m_texturePath = Property_<string>::create("Texture path", "smoketex.png");
        registerProperty(m_texturePath);
        
        m_point_color = Property_<vec4>::create("Point color", vec4(1));
        registerProperty(m_point_color);
        
        m_debug_draw = Property_<bool>::create("Enable debug drawing", true);
        registerProperty(m_debug_draw);
        
        m_use_syphon = Property_<bool>::create("Output to Syphon", false);
        registerProperty(m_use_syphon);
        
        m_syphon_server_name = Property_<std::string>::create("Syphon server name", "Poodackel");
        registerProperty(m_syphon_server_name);
        
        m_fbo_size = Property_<glm::vec2>::create("FBO size", vec2(640, 360));
        registerProperty(m_fbo_size);
        
        create_tweakbar_from_component(shared_from_this());
        observeProperties();
        
        m_pointMaterial = gl::Material::create(gl::createShader(gl::SHADER_POINTS_COLOR));
        m_pointMaterial->addTexture(gl::createTextureFromFile("smoketex.png"));
        m_pointMaterial->setPointSize(9.f);
        //m_pointMaterial->setDiffuse(vec4(1, 1, 1, .7f));
        //m_pointMaterial->setBlending();
        //m_pointMaterial->setDepthWrite(false);
        
        m_geom = gl::Geometry::create();
        m_geom->setPrimitiveType(GL_POINTS);
        m_mesh = gl::Mesh::create(m_geom, m_pointMaterial);
        
        initOpenCL();
        
        m_numParticles = 100000;
        GLsizei numBytes = m_numParticles * sizeof(vec4);
        
        m_geom->vertices().resize(m_numParticles, vec3(0));
        m_geom->colors().resize(m_numParticles, vec4(1));
        m_geom->point_sizes().resize(m_numParticles, 9.f);
        m_geom->createGLBuffers();
        
        m_mesh->material()->setPointSize(2.f);
        scene().addObject(m_mesh);
        try
        {
            // shared position buffer for OpenGL / OpenCL
            m_positions = cl::BufferGL(m_context, CL_MEM_READ_WRITE, m_geom->vertexBuffer().id());
            m_colors = cl::BufferGL(m_context, CL_MEM_READ_WRITE, m_geom->colorBuffer().id());
            
            //create the OpenCL only arrays
            m_velocities = cl::Buffer( m_context, CL_MEM_WRITE_ONLY, numBytes );
            m_positionGen = cl::Buffer( m_context, CL_MEM_WRITE_ONLY, numBytes );
            m_velocityGen = cl::Buffer( m_context, CL_MEM_WRITE_ONLY, numBytes );
            
            srand(clock());
            
            vector<vec4> posGen, velGen;
            for (int i = 0; i < m_numParticles; i++)
            {
                posGen.push_back( vec4(glm::ballRand(20.0f), 1.f) );
                vec2 tmp = glm::linearRand(vec2(-100), vec2(100));
                float life = kinski::random(2.f, 5.f);
                float yVel = kinski::random<float>(5, 15);
                velGen.push_back(vec4(tmp.x, yVel, tmp.y, life));
                m_geom->point_sizes()[i] = kinski::random(5.f, 15.f);
            }
            m_geom->createGLBuffers();
            
            m_queue.enqueueWriteBuffer(m_velocities, CL_TRUE, 0, numBytes, &velGen[0]);
            m_queue.enqueueWriteBuffer(m_positionGen, CL_TRUE, 0, numBytes, &posGen[0]);
            m_queue.enqueueWriteBuffer(m_velocityGen, CL_TRUE, 0, numBytes, &velGen[0]);
            
            m_particleKernel.setArg(0, m_positions);
            m_particleKernel.setArg(1, m_colors);
            m_particleKernel.setArg(2, m_velocities);
            m_particleKernel.setArg(3, m_positionGen);
            m_particleKernel.setArg(4, m_velocityGen);
        }
        catch(cl::Error &error)
        {
            LOG_ERROR << error.what() << "(" << error.err() << ")";
        }
        //Scene setup
        m_free_camera = gl::PerspectiveCamera::Ptr(new gl::PerspectiveCamera(16.f/9, 45.f, 50, 800));
        m_free_camera->setPosition(glm::vec3(0, 550, 0));
        m_free_camera->setLookAt(glm::vec3(0), glm::vec3(0,0,-1));
        m_free_camera_mesh = gl::createFrustumMesh(m_free_camera);
        scene().addObject(m_free_camera_mesh);
        
        // FBO
        m_fbo = gl::Fbo(640, 360);
        
        // syphon
        m_syphon = gl::SyphonConnector(*m_syphon_server_name);
        
        // OpenNI
        m_open_ni = gl::OpenNIConnector::Ptr(new gl::OpenNIConnector());
        m_open_ni->observeProperties();
        create_tweakbar_from_component(m_open_ni);
        
        // load state from config file
        try
        {
            Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
        }catch(Exception &e)
        {
            LOG_WARNING << e.what();
        }
    }
    
    void tearDown()
    {
        LOG_INFO<<"ciao openclTest";
    }
    
    void update(float timeDelta)
    {
        ViewerApp::update(timeDelta);
        updateParticles(timeDelta);
        setColors();
    }
    
    void draw()
    {
        if(*m_debug_draw)
        {
            gl::setMatrices(camera());
            if(draw_grid()) gl::drawGrid(1200, 1200);
            scene().render(camera());
            //gl::drawPoints(m_geom->vertexBuffer().id(), m_numParticles, gl::MaterialPtr(), sizeof(vec4));
        }
        
        m_textures[1] = render_to_texture(scene(), m_free_camera);
        
        if(*m_use_syphon)
        {
            m_syphon.publish_texture(m_textures[1]);
            //m_syphon.publish_framebuffer(m_fbo);
        }
        
        if(displayTweakBar())
        {
            // draw opencv maps
            float w = (windowSize()/8.f).x;
            glm::vec2 offset(getWidth() - w - 10, 10);
            for(int i = 0;i < 2;i++)
            {
                float h = m_textures[i].getHeight() * w / m_textures[i].getWidth();
                glm::vec2 step(0, h + 10);
                drawTexture(m_textures[i], vec2(w, h), offset);
                gl::drawText2D(as_string(m_textures[i].getWidth()) + std::string(" x ") +
                               as_string(m_textures[i].getHeight()), m_font, glm::vec4(1),
                               offset);
                offset += step;
            }
        }
        // draw fps string
        gl::drawText2D(kinski::as_string(framesPerSec()), m_font,
                       vec4(vec3(1) - clear_color().xyz(), 1.f),
                       glm::vec2(windowSize().x - 115, windowSize().y - 30));
    }
    
    void updateProperty(const Property::ConstPtr &theProperty)
    {
        ViewerApp::updateProperty(theProperty);
        
        // one of our properties has changed
        if(theProperty == m_texturePath)
        {
            try
            {
                m_textures[0] = gl::createTextureFromFile(*m_texturePath);
                
                // ->CL_INVALID_GL_OBJECT: internal format must be pow2 (RG, RGBA)
                m_cl_image = cl::ImageGL(m_context, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0,
                                           m_textures[0].getId());
            }
            catch(cl::Error &error){LOG_ERROR << error.what() << "(" << error.err() << ")";}
            catch (std::exception &e){LOG_WARNING << e.what();}
        }
        else if(theProperty == m_point_color)
        {
            m_mesh->material()->setDiffuse(*m_point_color);
        }
        else if(theProperty == m_use_syphon)
        {
            //TODO: narrow down cause of error here
            //m_syphon = *m_use_syphon ? gl::SyphonConnector(*m_syphon_server_name) : gl::SyphonConnector();
        }
        else if(theProperty == m_syphon_server_name)
        {
            m_syphon.setName(*m_syphon_server_name);
        }
        else if(theProperty == m_fbo_size)
        {
            m_fbo = gl::Fbo(m_fbo_size->value().x, m_fbo_size->value().y);
            m_free_camera->setAspectRatio(m_fbo_size->value().x / m_fbo_size->value().y);
            scene().removeObject(m_free_camera_mesh);
            m_free_camera_mesh = gl::createFrustumMesh(m_free_camera);
            scene().addObject(m_free_camera_mesh);
        }
    }
    
    gl::Texture render_to_texture(const gl::Scene &theScene, const gl::CameraPtr theCam)
    {
        // push framebuffer and viewport states
        gl::SaveViewPort sv; gl::SaveFramebufferBinding sfb;
        gl::setWindowDimension(m_fbo.getSize());
        m_fbo.bindFramebuffer();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        theScene.render(theCam);
        return m_fbo.getTexture();
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new OpenCLTest);
    return theApp->run();
}
