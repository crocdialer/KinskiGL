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
    gl::MeshPtr m_particle_mesh;
    gl::Font m_font;
    
    //OpenCL standard stuff
    cl::Context m_context;
    cl::Device m_device;
    cl::CommandQueue m_queue;
    cl::Program m_program;
    
    // particle system related
    RangedProperty<int>::Ptr m_numParticles;
    cl::Kernel m_particleKernel, m_imageKernel;
    cl::Buffer m_velocities, m_positionGen, m_velocityGen, m_user_positions;
    cl::BufferGL m_positions, m_colors, m_point_sizes;
    cl::ImageGL m_cl_image;
    
    // perspective experiment
    gl::Scene m_debug_scene;
    gl::PerspectiveCamera::Ptr m_free_camera;
    gl::MeshPtr m_free_camera_mesh;
    gl::Fbo m_fbo;
    Property_<glm::vec2>::Ptr m_fbo_size;
    RangedProperty<float>::Ptr m_fbo_cam_distance;
    gl::MeshPtr m_user_mesh;
    std::vector<gl::Color> m_user_id_colors;
    
    // output via Syphon
    gl::SyphonConnector m_syphon;
    Property_<bool>::Ptr m_debug_draw;
    Property_<bool>::Ptr m_use_syphon;
    Property_<std::string>::Ptr m_syphon_server_name;
    
    // OpenNI interface
    gl::OpenNIConnector::Ptr m_open_ni;
    gl::OpenNIConnector::UserList m_user_list;
    gl::PerspectiveCamera::Ptr m_depth_cam;
    gl::MeshPtr m_depth_cam_mesh;

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
    
    void initParticles(uint32_t num_particles)
    {
        // Particle system
        m_pointMaterial = gl::Material::create(gl::createShader(gl::SHADER_POINTS_COLOR));
        m_pointMaterial->addTexture(gl::createTextureFromFile("smoketex.png"));
        m_pointMaterial->setPointSize(9.f);
        m_geom = gl::Geometry::create();
        m_geom->setPrimitiveType(GL_POINTS);
        m_particle_mesh = gl::Mesh::create(m_geom, m_pointMaterial);
        m_particle_mesh->transform()[3].y = 300;
        
        GLsizei numBytes = num_particles * sizeof(vec4);
        
        m_geom->vertices().resize(num_particles, vec3(0));
        m_geom->colors().resize(num_particles, vec4(1));
        m_geom->point_sizes().resize(num_particles, 9.f);
        m_geom->createGLBuffers();
        
        m_particle_mesh->material()->setPointSize(2.f);
        scene().addObject(m_particle_mesh);
        m_debug_scene.addObject(m_particle_mesh);
        try
        {
            // shared buffers for OpenGL / OpenCL
            m_positions = cl::BufferGL(m_context, CL_MEM_READ_WRITE, m_geom->vertexBuffer().id());
            m_colors = cl::BufferGL(m_context, CL_MEM_READ_WRITE, m_geom->colorBuffer().id());
            m_point_sizes = cl::BufferGL(m_context, CL_MEM_READ_WRITE, m_geom->pointSizeBuffer().id());
            
            //create the OpenCL only arrays
            m_velocities = cl::Buffer(m_context, CL_MEM_WRITE_ONLY, numBytes );
            m_positionGen = cl::Buffer(m_context, CL_MEM_WRITE_ONLY, numBytes );
            m_velocityGen = cl::Buffer(m_context, CL_MEM_WRITE_ONLY, numBytes );
            m_user_positions = cl::Buffer(m_context, CL_MEM_WRITE_ONLY,
                                          200 * sizeof(gl::OpenNIConnector::User));
            srand(clock());
            
            vector<vec4> posGen, velGen;
            vec3 image_dim = vec3(m_textures[0].getWidth(), m_textures[0].getHeight(), 0);
            for (int i = 0; i < num_particles; i++)
            {
                vec3 pos = glm::linearRand(-image_dim/2.f, image_dim/2.f);//glm::ballRand(20.0f);
                posGen.push_back( vec4(pos, 1.f) );
                //vec2 tmp = glm::linearRand(vec2(-100), vec2(100));
                vec3 vel = glm::vec3(random(-20.f, 20.f),random(-6.f, 6.f), random(0.f, 4.f));
                float life = kinski::random(5.f, 18.f);
                velGen.push_back(vec4(vel, life));
                m_geom->point_sizes()[i] = kinski::random(5.f, 8.f);
            }
            m_geom->vertices()[0] = vec3(-1000); m_geom->vertices()[1] = vec3(1000);
            m_geom->computeBoundingBox();
            m_geom->createGLBuffers();
            
            m_queue.enqueueWriteBuffer(m_velocities, CL_TRUE, 0, numBytes, &velGen[0]);
            m_queue.enqueueWriteBuffer(m_positionGen, CL_TRUE, 0, numBytes, &posGen[0]);
            m_queue.enqueueWriteBuffer(m_velocityGen, CL_TRUE, 0, numBytes, &velGen[0]);
            
            m_particleKernel.setArg(0, m_positions);
            m_particleKernel.setArg(1, m_colors);
            m_particleKernel.setArg(2, m_velocities);
            m_particleKernel.setArg(3, m_positionGen);
            m_particleKernel.setArg(4, m_velocityGen);
            m_particleKernel.setArg(5, 0.0f);
            m_particleKernel.setArg(6, m_user_positions);
            m_particleKernel.setArg(7, 0);
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
            //            vector<cl::Memory> glBuffers;
            //            glBuffers.push_back(m_positions);
            //            glBuffers.push_back(m_colors);
            //            glFinish();
            //            m_queue.enqueueAcquireGLObjects(&glBuffers);
            
            if(!m_user_list.empty())
            {
                std::vector<vec4> positions_vector;
                gl::OpenNIConnector::UserList::iterator it = m_user_list.begin();
                for(;it != m_user_list.end();++it)
                {
                    positions_vector.push_back(vec4(it->position, 1.f));
                }
                
//                m_queue.enqueueWriteBuffer(m_user_positions, CL_TRUE, 0,
//                                           m_user_list.size() * sizeof(m_user_list[0]), &m_user_list[0]);
                
                m_queue.enqueueWriteBuffer(m_user_positions, CL_TRUE, 0,
                                           positions_vector.size() * sizeof(positions_vector[0]),
                                           &positions_vector[0]);
            }
            m_particleKernel.setArg(7, m_user_list.size());

            m_particleKernel.setArg(5, timeDelta); //pass in the timestep
            
            //execute the kernel
            m_queue.enqueueNDRangeKernel(m_particleKernel, cl::NullRange, cl::NDRange(*m_numParticles),
                                         cl::NullRange);
            
            //            m_queue.enqueueReleaseGLObjects(&glBuffers, NULL);
            //            m_queue.finish();
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
//            vector<cl::Memory> glBuffers;
//            glBuffers.push_back(m_positions);
//            glBuffers.push_back(m_colors);
//            glBuffers.push_back(m_cl_image);
//            glFinish();
//            m_queue.enqueueAcquireGLObjects(&glBuffers);
            
            m_imageKernel.setArg(0, m_cl_image);
            m_imageKernel.setArg(1, m_positions);
            m_imageKernel.setArg(2, m_colors);
            
            //execute the kernel
            m_queue.enqueueNDRangeKernel(m_imageKernel, cl::NullRange, cl::NDRange(*m_numParticles),
                                         cl::NullRange);
            
//            m_queue.enqueueReleaseGLObjects(&glBuffers, NULL);
//            m_queue.finish();
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
        
        m_fbo_cam_distance = RangedProperty<float>::create("FBO cam distance", 550.f, 0, 5000);
        registerProperty(m_fbo_cam_distance);
        
        m_numParticles = RangedProperty<int>::create("Num particles", 100000, 1, 5000000);
        registerProperty(m_numParticles);
        
        create_tweakbar_from_component(shared_from_this());
        observeProperties();
        
        initOpenCL();
        
        // init the particle system
        initParticles(10000);
        
        //Scene setup
        camera()->setClippingPlanes(.1f, 7500.f);
        
        // the camera used for offscreen rendering
        m_free_camera = gl::PerspectiveCamera::Ptr(new gl::PerspectiveCamera(16.f/9, 45.f, 50, 1500));
        m_free_camera->setPosition( m_particle_mesh->position() + vec3(0, 0, *m_fbo_cam_distance));
        m_free_camera->setLookAt(m_particle_mesh->position(), glm::vec3(0, 1, 0));
        m_free_camera_mesh = gl::createFrustumMesh(m_free_camera);
        m_debug_scene.addObject(m_free_camera_mesh);
        
        // FBO
        m_fbo = gl::Fbo(640, 360);
        
        // syphon
        m_syphon = gl::SyphonConnector(*m_syphon_server_name);
        
        // OpenNI
        m_open_ni = gl::OpenNIConnector::Ptr(new gl::OpenNIConnector());
        m_open_ni->observeProperties();
        create_tweakbar_from_component(m_open_ni);
        
        // the camera used to calibrate depth camera input
        m_depth_cam = gl::PerspectiveCamera::Ptr(new gl::PerspectiveCamera(4/3.f, 2 * 45.f, 100.f, 1200.f));
        m_depth_cam->setTransform(glm::rotate(mat4(), 180.f, vec3(0, 1, 0)));
        m_depth_cam_mesh = gl::createFrustumMesh(m_depth_cam);
        m_depth_cam_mesh->material()->setDiffuse(gl::Color(1, 0, 0, 1));
        m_debug_scene.addObject(m_depth_cam_mesh);
        
        // random user colors
        m_user_id_colors.resize(50);
        for (int i= 0; i < m_user_id_colors.size(); ++i)
        {m_user_id_colors[i] = gl::Color(random(0.f, 1.f), .2f, random(0.f, 1.f), 1.f);}
        
        // load state from config file
        try
        {
            Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
            Serializer::loadComponentState(m_open_ni, "ni_config.json", PropertyIO_GL());
        }catch(Exception &e)
        {
            LOG_WARNING << e.what();
        }
    }
    
    void tearDown()
    {
        LOG_DEBUG<<"waiting for OpenNI to shut down";
        m_open_ni->stop();
        LOG_INFO<<"ciao openclTest";
    }
    
    void update(float timeDelta)
    {
        ViewerApp::update(timeDelta);
        
        // query user positions from OpenNI
        m_user_list = m_open_ni->get_user_positions();
        
        // calibrate camera: bring positions to world-coords
        mat4 inverse_depth_cam_mat;// = glm::inverse(m_depth_cam->transform());
        mat4 inverse_model_mat;// = glm::inverse(m_particle_mesh->transform());
        
        gl::OpenNIConnector::UserList::iterator it = m_user_list.begin();
        for(;it != m_user_list.end();++it)
        {
            vec4 flipped_pos (it->position, 1.f);// flipped_pos.xy() = (-1.f) * vec2(flipped_pos.xy());
            it->position = (inverse_model_mat * inverse_depth_cam_mat * flipped_pos).xyz();
        }
        
        // OpenCL updates
        try
        {
            vector<cl::Memory> glBuffers;
            glBuffers.push_back(m_positions);
            glBuffers.push_back(m_colors);
            glBuffers.push_back(m_cl_image);
            glFinish();
            m_queue.enqueueAcquireGLObjects(&glBuffers);
            
            setColors();
            updateParticles(timeDelta);
            
            m_queue.enqueueReleaseGLObjects(&glBuffers, NULL);
            m_queue.finish();
        }
        catch(cl::Error &error)
        {
            LOG_ERROR << error.what() << "(" << error.err() << ")";
        }
    }
    
    void draw()
    {
        // render the output image offscreen
        m_textures[1] = render_to_texture(scene(), m_free_camera);
        
        if(*m_debug_draw)
        {
            gl::setMatrices(camera());
            if(draw_grid()) gl::drawGrid(3600, 3600);
            m_debug_scene.render(camera());
            draw_user_meshes(m_user_list);
            //gl::drawPoints(m_geom->vertexBuffer().id(), m_numParticles, gl::MaterialPtr(), sizeof(vec4));
        }
        else
        {
            gl::drawTexture(m_textures[1], windowSize());
        }

        if(*m_use_syphon)
        {
            m_syphon.publish_texture(m_textures[1]);
            //m_syphon.publish_framebuffer(m_fbo);
        }
        
        if(displayTweakBar())
        {
            // draw opencv maps
            float w = (windowSize()/6.f).x;
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
                       gl::Color(vec3(1) - clear_color().xyz(), 1.f),
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
            m_particle_mesh->material()->setDiffuse(*m_point_color);
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
        else if(theProperty == m_fbo_size || theProperty == m_fbo_cam_distance)
        {
            m_fbo = gl::Fbo(m_fbo_size->value().x, m_fbo_size->value().y);
            m_free_camera->setAspectRatio(m_fbo_size->value().x / m_fbo_size->value().y);
            m_free_camera->setPosition(m_particle_mesh->position() + vec3(0, 0, *m_fbo_cam_distance));
            m_debug_scene.removeObject(m_free_camera_mesh);
            m_free_camera_mesh = gl::createFrustumMesh(m_free_camera);
            m_debug_scene.addObject(m_free_camera_mesh);
        }
        else if(theProperty == m_numParticles)
        {
            scene().removeObject(m_particle_mesh);
            m_debug_scene.removeObject(m_particle_mesh);
            initParticles(*m_numParticles);
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
    
    void draw_user_meshes(const gl::OpenNIConnector::UserList &user_list)
    {
        if(!m_user_mesh)
        {
            m_user_mesh = gl::Mesh::create(gl::createSolidUnitCircle(64), gl::Material::create());
            m_user_mesh->material()->setTwoSided();
            //m_user_mesh->material()->setDepthTest(false);
            m_user_mesh->material()->setDiffuse(gl::Color(1, 0, 0, 1));
            m_user_mesh->transform() *= glm::scale(glm::mat4(), glm::vec3(100));
            m_user_mesh->transform() *= glm::rotate(glm::mat4(), -90.f, glm::vec3(1, 0, 0));
        }
        gl::loadMatrix(gl::PROJECTION_MATRIX, camera()->getProjectionMatrix());
        gl::OpenNIConnector::UserList::const_iterator it = user_list.begin();
        for (; it != user_list.end(); ++it)
        {
            m_user_mesh->setPosition(it->position);
            m_user_mesh->transform()[3].y = 5;
            m_user_mesh->material()->setDiffuse(m_user_id_colors[it->id]);
            gl::loadMatrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix() * m_user_mesh->transform());
            gl::drawMesh(m_user_mesh);
        }
    }
    
    void keyPress(const KeyEvent &e)
    {
        ViewerApp::keyPress(e);
        
        switch(e.getChar())
        {
            case KeyEvent::KEY_d:
                *m_debug_draw = !*m_debug_draw;
                break;
            case KeyEvent::KEY_s:
                Serializer::saveComponentState(m_open_ni, "ni_config.json", PropertyIO_GL());
                break;
            case KeyEvent::KEY_r:
                Serializer::loadComponentState(m_open_ni, "ni_config.json", PropertyIO_GL());
                break;
        }
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new OpenCLTest);
    return theApp->run();
}
