//
//  ParticleSystem.cpp
//  gl
//
//  Created by Fabian on 18/07/14.
//
//

#include "ParticleSystem.hpp"
#include "gl/Mesh.hpp"

using namespace std;
using namespace glm;

namespace kinski{ namespace gl{
    
    struct Params
    {
        vec4 gravity, contraints_min, contraints_max;
        float bouncyness;
    };
    
    ParticleSystem::ParticleSystem():
    m_gravity(vec3(0, -9.87f, 0)),
    m_particle_bounce(0.f),
    m_use_constraints(true)
    {
        
    }
    
    ParticleSystem::ParticleSystem(const cl_context& context):
    m_opencl(context),
    m_gravity(vec3(0, -9.87f, 0)),
    m_particle_bounce(0.f),
    m_use_constraints(true)
    {
    
    }
    
    void ParticleSystem::set_mesh(gl::MeshPtr the_mesh)
    {
        m_mesh = the_mesh;
        
        if(m_mesh)
        {
            auto &geom = m_mesh->geometry();
            geom->create_gl_buffers();
            the_mesh->create_vertex_attribs();
            
            m_contraints_aabb = geom->bounding_box();
            
            try
            {
                // shared position buffer for OpenGL / OpenCL
                if(!geom->vertices().empty())
                {                
                    m_vertices = cl::BufferGL(m_opencl.context(), CL_MEM_READ_WRITE,
                                              geom->vertex_buffer().id());
                }
                
                if(geom->has_colors())
                {
                    m_colors = cl::BufferGL(m_opencl.context(), CL_MEM_READ_WRITE,
                                            geom->color_buffer().id());
                }
                if(geom->has_normals())
                {
                    m_normals = cl::BufferGL(m_opencl.context(), CL_MEM_READ_WRITE,
                                             geom->normal_buffer().id());
                }
                if(geom->has_tex_coords())
                {
                    m_texCoords = cl::BufferGL(m_opencl.context(), CL_MEM_READ_WRITE,
                                               geom->tex_coord_buffer().id());
                }
                if(geom->has_point_sizes())
                {
                    m_pointSizes = cl::BufferGL(m_opencl.context(), CL_MEM_READ_WRITE,
                                                geom->point_size_buffer().id());
                }
                
                //////////////// create the OpenCL only arrays //////////////////
                
                // combined velocity / life array
                m_velocities = cl::Buffer(m_opencl.context(), CL_MEM_READ_WRITE,
                                          geom->vertex_buffer().num_bytes());
                
                // spawn positions
                m_positionGen = cl::Buffer(m_opencl.context(), CL_MEM_READ_WRITE,
                                           geom->vertex_buffer().num_bytes() );
                
                
                m_velocityGen = cl::Buffer(m_opencl.context(), CL_MEM_READ_WRITE,
                                           geom->vertex_buffer().num_bytes());
                
                
                // reserve memory for 200 forces, seems enough
                m_force_buffer = cl::Buffer(m_opencl.context(), CL_MEM_READ_WRITE,
                                            200 * sizeof(glm::vec4));
                
                
                m_param_buffer = cl::Buffer(opencl().context(), CL_MEM_READ_ONLY, sizeof(Params), NULL);
                
                vector<glm::vec4> velGen;
                
                for (int i = 0; i < num_particles(); i++)
                {
                    float life = 1000.f;//kinski::random(2.f, 5.f);
                    glm::vec3 vel = vec3(0);//glm::linearRand(glm::vec3(-100), glm::vec3(100));
                    velGen.push_back(glm::vec4(vel, life));
                }
                
                // all buffer are holding vec4s and have same size in bytes
                int num_bytes = geom->vertex_buffer().num_bytes();
                
                m_opencl.queue().enqueueWriteBuffer(m_velocities, CL_TRUE, 0, num_bytes, &velGen[0]);
                m_opencl.queue().enqueueWriteBuffer(m_velocityGen, CL_TRUE, 0, num_bytes, &velGen[0]);
                
                // generate spawn positions from original positions
                uint8_t *vert_buf = geom->vertex_buffer().map();
                m_opencl.queue().enqueueWriteBuffer(m_positionGen, CL_TRUE, 0,
                                                    geom->vertex_buffer().num_bytes(),
                                                    vert_buf);
                geom->vertex_buffer().unmap();
            }
            catch(cl::Error &error)
            {
                LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")";
            }
        }
    
    }
    
    void ParticleSystem::update(float time_delta)
    {
//        // update global param buffer
//        Params params;
//        params.gravity = vec4(m_gravity, 0);
//        
//        params.contraints_min = m_mesh->transform() * (m_use_constraints ? vec4(m_contraints_aabb.min, 1) :
//            vec4(std::numeric_limits<float>::lowest()));
//        
//        params.contraints_max = m_mesh->transform() * (m_use_constraints ? vec4(m_contraints_aabb.max, 1) :
//            vec4(std::numeric_limits<float>::max()));
//        params.bouncyness = m_particle_bounce;
//        
//        m_opencl.queue().enqueueWriteBuffer(m_param_buffer, CL_TRUE, 0, sizeof(Params), &params);
        
        // apply our forces
        apply_forces(time_delta);
        
        auto iter = m_kernel_map.find("updateParticles");
        if(iter != m_kernel_map.end())
        {
            // get a ref for our kernel
            auto &kernel = iter->second;
            
            try
            {
                vector<cl::Memory> glBuffers = {m_vertices, m_colors, m_pointSizes};
                
                // Make sure OpenGL is done using our VBOs
                glFinish();
                
                // map OpenGL buffer object for writing from OpenCL
                // this passes in the vector of VBO buffer objects (position and color)
                m_opencl.queue().enqueueAcquireGLObjects(&glBuffers);
                
                kernel.setArg(0, m_vertices);
                kernel.setArg(1, m_colors);
                kernel.setArg(2, m_pointSizes);
                kernel.setArg(3, m_velocities);
                kernel.setArg(4, m_positionGen);
                kernel.setArg(5, m_velocityGen);
                kernel.setArg(6, time_delta); //pass in the timestep
                kernel.setArg(7, m_param_buffer);
                
                int num = num_particles();
                
                // execute the kernel
                m_opencl.queue().enqueueNDRangeKernel(kernel,
                                                      cl::NullRange,
                                                      cl::NDRange(num),
                                                      cl::NullRange);
                
                // Release the VBOs again
                m_opencl.queue().enqueueReleaseGLObjects(&glBuffers, NULL);
                
                m_opencl.queue().finish();
            }
            catch(cl::Error &error)
            {
                LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")";
            }
        }
    }
    
    void ParticleSystem::texture_input(gl::Texture &the_texture)
    {
        auto iter = m_kernel_map.find("texture_input");
        if(iter != m_kernel_map.end())
        {
            // get a ref for our kernel
            auto &kernel = iter->second;
            
            try
            {
                cl::ImageGL img(opencl().context(), CL_MEM_READ_ONLY, the_texture.target(), 0,
                                the_texture.id());
                
                kernel.setArg(0, img);
                kernel.setArg(1, m_positionGen);
                kernel.setArg(2, m_param_buffer);
                
                int num = num_particles();
                
                // execute the kernel
                m_opencl.queue().enqueueNDRangeKernel(kernel,
                                                      cl::NullRange,
                                                      cl::NDRange(num),
                                                      cl::NullRange);
                
                m_opencl.queue().finish();
            }
            catch(cl::Error &error)
            {
                LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")";
            }
            
        }
    }
    
    void ParticleSystem::texture_input_alt(gl::Texture &the_tex1, gl::Texture &the_tex2)
    {
        auto iter = m_kernel_map.find("texture_input_alt");
        if(iter != m_kernel_map.end())
        {
            // get a ref for our kernel
            auto &kernel = iter->second;
            
            try
            {
                cl::ImageGL img1(opencl().context(), CL_MEM_READ_ONLY, the_tex1.target(), 0,
                                the_tex1.id());
                
                cl::ImageGL img2(opencl().context(), CL_MEM_READ_ONLY, the_tex2.target(), 0,
                                 the_tex2.id());
                
                kernel.setArg(0, img1);
                kernel.setArg(1, img2);
                kernel.setArg(2, m_positionGen);
                kernel.setArg(3, m_param_buffer);
                
                int num = num_particles();
                
                // execute the kernel
                m_opencl.queue().enqueueNDRangeKernel(kernel,
                                                      cl::NullRange,
                                                      cl::NDRange(num),
                                                      cl::NullRange);
                
                m_opencl.queue().finish();
            }
            catch(cl::Error &error)
            {
                LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")";
            }
            
        }
    }
    
    void ParticleSystem::apply_forces(float time_delta)
    {
        auto iter = m_kernel_map.find("apply_forces");
        if(iter != m_kernel_map.end())
        {
            // get a ref for our kernel
            auto &force_kernel = iter->second;
            
            try
            {
                if(!m_forces.empty())
                {
                    m_opencl.queue().enqueueWriteBuffer(m_force_buffer, CL_TRUE, 0,
                                                        m_forces.size() * sizeof(vec4),
                                                        &m_forces[0]);
                }
                
                force_kernel.setArg(0, m_vertices);
                force_kernel.setArg(1, m_velocities);
                force_kernel.setArg(2, m_force_buffer);
                force_kernel.setArg(3, (int)m_forces.size());
                force_kernel.setArg(4, time_delta);
                force_kernel.setArg(5, m_param_buffer);
                
                int num = num_particles();
                
                // execute the kernel
                m_opencl.queue().enqueueNDRangeKernel(force_kernel,
                                                      cl::NullRange,
                                                      cl::NDRange(num),
                                                      cl::NullRange);
                
                m_opencl.queue().finish();
            }
            catch(cl::Error &error)
            {
                LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")";
            }
        
        }
    }
    
    int ParticleSystem::num_particles() const
    {
        int ret = 0;
        
        if(m_mesh)
            ret = m_mesh->geometry()->vertices().size();
            
        return ret;
    }
    
    void ParticleSystem::add_kernel(const std::string &kernel_name)
    {
        // OpenCL
        try
        {
            auto pair = std::make_pair(kernel_name, cl::Kernel(opencl().program(),
                                                               kernel_name.c_str()));
            
            m_kernel_map.insert(pair);
        }
        catch(cl::Error &error)
        {
            LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")";
        }
    }
    
    void ParticleSystem::set_gravity(const glm::vec3 &the_gravity)
    {
        m_gravity = the_gravity;
    }
    
    const glm::vec3& ParticleSystem::gravity() const
    {
        return m_gravity;
    }
    
    void ParticleSystem::set_param_buffer(void *the_data, size_t num_bytes)
    {
        m_opencl.queue().enqueueWriteBuffer(m_param_buffer, CL_TRUE, 0, num_bytes, the_data);
    }
}}
