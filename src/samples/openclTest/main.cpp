#include "kinskiApp/ViewerApp.h"

//#define __NO_STD_VECTOR
#define __CL_ENABLE_EXCEPTIONS
#include "cl_context.h"

#include "kinskiApp/LightComponent.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>

using namespace std;
using namespace kinski;
using namespace glm;

class OpenCLTest : public ViewerApp
{
public:
    OpenCLTest():ViewerApp(), m_timer(io_service(), boost::posix_time::seconds(1.f))
    {
        m_timer.async_wait(boost::bind(&OpenCLTest::tick, this, _1));
    };
    
    void tick(const boost::system::error_code& error)
    {
        if(!error)
        {
            LOG_DEBUG<<"io_service -> tick...";
        }
//        m_timer.expires_at(m_timer.expires_at() + boost::posix_time::seconds(1.f));
//        m_timer.async_wait(boost::bind(&OpenCLTest::tick, this));
    };
    
private:
    
    Property_<string>::Ptr m_texturePath;
    Property_<string>::Ptr m_videoPath;
    RangedProperty<int>::Ptr m_num_particles;
    RangedProperty<float>::Ptr m_point_size;
    Property_<vec4>::Ptr m_point_color;
    gl::Texture m_textures[4];
    gl::Material::Ptr m_pointMaterial;
    gl::GeometryPtr m_geom;
    gl::MeshPtr m_mesh;
    gl::Font m_font;
    
    // OpenCL standard stuff
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
    
    LightComponent::Ptr m_light_component;
    
    boost::asio::deadline_timer m_timer;
    
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
            LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")";
            LOG_ERROR << "Build Status: " << m_program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(m_device);
            LOG_ERROR << "Build Options:\t" << m_program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(m_device);
            LOG_ERROR << "Build Log:\t " << m_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(m_device);
        }
    }
    
    void initParticles(uint32_t num_particles)
    {
        m_geom = gl::Geometry::create();
        m_geom->setPrimitiveType(GL_POINTS);
        m_mesh = gl::Mesh::create(m_geom, m_pointMaterial);
        
        m_numParticles = num_particles;
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
            LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")";
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
            LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")";
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
            
            //this will update our system by calculating new velocity and updating the positions of our particles
            //Make sure OpenGL is done using our VBOs
            glFinish();
            
            // map OpenGL buffer object for writing from OpenCL
            // this passes in the vector of VBO buffer objects (position and color)
            m_queue.enqueueAcquireGLObjects(&glBuffers);
            
            
            m_imageKernel.setArg(0, m_cl_image);
            m_imageKernel.setArg(1, m_positions);
            m_imageKernel.setArg(2, m_colors);
            
            //execute the kernel
            m_queue.enqueueNDRangeKernel(m_imageKernel, cl::NullRange, cl::NDRange(m_numParticles),
                                         cl::NullRange);
            
            
            //Release the VBOs so OpenGL can play with them
            m_queue.enqueueReleaseGLObjects(&glBuffers, NULL);
            
            m_queue.finish();
        }
        catch(cl::Error &error)
        {
            LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")";
        }
    }
    
public:
    
    void setup()
    {
        ViewerApp::setup();
        kinski::addSearchPath("/Library/Fonts");
        kinski::addSearchPath("~/Pictures");
        
        m_font.load("Courier New Bold.ttf", 24);
        outstream_gl().set_font(m_font);
        
        m_texturePath = Property_<string>::create("Texture path", "smoketex.png");
        registerProperty(m_texturePath);
        
        m_videoPath = Property_<string>::create("Video path", "");
        registerProperty(m_videoPath);
        
        m_num_particles = RangedProperty<int>::create("Num particles", 100000, 1, 2000000);
        registerProperty(m_num_particles);
        
        m_point_size = RangedProperty<float>::create("Point size", 3.f, 1.f, 64.f);
        registerProperty(m_point_size);
        
        m_point_color = Property_<vec4>::create("Point color", vec4(1));
        registerProperty(m_point_color);
        
        observeProperties();
        create_tweakbar_from_component(shared_from_this());
        
        m_pointMaterial = gl::Material::create(gl::createShader(gl::SHADER_POINTS_SPHERE));
        //m_pointMaterial->addTexture(gl::createTextureFromFile("smoketex.png"));
        float vals[2];
        glGetFloatv(GL_POINT_SIZE_RANGE, vals);
        m_point_size->setRange(vals[0], vals[1]);
        m_pointMaterial->setPointSize(*m_point_size);
        m_pointMaterial->setPointAttenuation(0.f, 0.01f, 0.f);
        m_pointMaterial->uniform("u_pointRadius", 50.f);
        
        m_pointMaterial->setDiffuse(vec4(1, 1, 1, .7f));
        m_pointMaterial->setBlending();
//        m_pointMaterial->setDepthWrite(false);
//        m_pointMaterial->addTexture(gl::createTextureFromFile("~/Desktop/harp_icon.png"));
        
        initOpenCL();
        initParticles(*m_num_particles);
        
        // light component
        m_light_component.reset(new LightComponent());
        m_light_component->set_lights(lights());
        m_light_component->refresh();
        create_tweakbar_from_component(m_light_component);
        
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
        m_pointMaterial->uniform("u_lightDir", light_direction());
        
        ViewerApp::update(timeDelta);
        updateParticles(timeDelta);
        //setColors();
    }
    
    void draw()
    {
        gl::setMatrices(camera());
        if(draw_grid()) gl::drawGrid(200, 200);
        scene().render(camera());
        
        //gl::drawPoints(m_geom->vertexBuffer().id(), m_numParticles, gl::MaterialPtr(), sizeof(vec4));
        
        if(displayTweakBar())
        {
            // draw opencv maps
            float w = (windowSize()/8.f).x;
            glm::vec2 offset(getWidth() - w - 10, 10);
            for(int i = 0;i < 2;i++)
            {
                gl::Texture &t = m_textures[i];
                
                if(!t) continue;
                
                float h = t.getHeight() * w / t.getWidth();
                glm::vec2 step(0, h + 10);
                drawTexture(t, vec2(w, h), offset);
                gl::drawText2D(as_string(t.getWidth()) + std::string(" x ") +
                               as_string(t.getHeight()), m_font, glm::vec4(1),
                               offset);
                offset += step;
            }
            
            // draw fps string
            gl::drawText2D(kinski::as_string(framesPerSec()), m_font,
                           vec4(vec3(1) - clear_color().xyz(), 1.f),
                           glm::vec2(windowSize().x - 115, windowSize().y - 30));
        }
    }
    
    void updateProperty(const Property::ConstPtr &theProperty)
    {
        ViewerApp::updateProperty(theProperty);
        
        // one of our porperties was changed
        if(theProperty == m_texturePath)
        {
            try
            {
                m_textures[0] = gl::createTextureFromFile(*m_texturePath);
                
                if(m_textures[0])
                {
                    // ->CL_INVALID_GL_OBJECT: internal format must be pow2 (RG, RGBA)
                    m_cl_image = cl::ImageGL(m_context, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0,
                                             m_textures[0].getId());
                }
            }
            catch(cl::Error &error){LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")";}
            catch (FileNotFoundException &e){LOG_WARNING << e.what();}
        }
        else if(theProperty == m_videoPath)
        {
            
        }
        else if(theProperty == m_num_particles)
        {
            scene().removeObject(m_mesh);
            initParticles(*m_num_particles);
        }
        else if(theProperty == m_point_size)
        {
            m_pointMaterial->setPointSize(*m_point_size);
            
            m_timer.cancel();
            m_timer.expires_from_now(boost::posix_time::seconds(2.f));
            m_timer.async_wait(boost::bind(&OpenCLTest::tick, this, _1));
        }
        else if(theProperty == m_point_color)
        {
            m_mesh->material()->setDiffuse(*m_point_color);
        }
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new OpenCLTest);
    return theApp->run();
}
