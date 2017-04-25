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

    namespace
    {
        const std::string g_update_kernel = "update_particles";
        const std::string g_forces_kernel = "apply_forces";
        const std::string g_constraints_kernel = "apply_contraints";
    }

    struct Params
    {
        vec4 gravity;
        vec4 velocity_min, velocity_max;
        float bouncyness;
        float life_min, life_max;
    };
    
    ParticleSystem::ParticleSystem(): ParticleSystem(cl_context())
    {
        
    }
    
    ParticleSystem::ParticleSystem(const cl_context& context):
    m_opencl(context),
    m_gravity(vec3(0, -9.87f, 0)),
    m_start_velocity_min(0),
    m_start_velocity_max(0),
    m_lifetime_min(1.f),
    m_lifetime_max(1.f),
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

                m_plane_buffer = cl::Buffer(m_opencl.context(), CL_MEM_READ_WRITE,
                                            200 * sizeof(gl::Plane));

                vector<glm::vec4> velGen;
                
                for (int i = 0; i < num_particles(); i++)
                {
                    float life = kinski::random(m_lifetime_min, m_lifetime_max);
                    glm::vec3 vel = glm::linearRand(m_start_velocity_min, m_start_velocity_max);
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
        update_params();
        
        auto iter = m_kernel_map.find(g_update_kernel);

        if(iter == m_kernel_map.end()){ LOG_WARNING << "kernel: " << g_update_kernel << " not found"; }
        else
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

                // apply our forces
                apply_forces(time_delta);

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
                m_opencl.queue().enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(num), cl::NullRange);

                // apply our constraints
                if(m_use_constraints){ apply_contraints(); }

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
    
//    void ParticleSystem::texture_input(gl::Texture &the_texture)
//    {
//        auto iter = m_kernel_map.find("texture_input");
//        if(iter != m_kernel_map.end())
//        {
//            // get a ref for our kernel
//            auto &kernel = iter->second;
//
//            try
//            {
//                cl::ImageGL img(opencl().context(), CL_MEM_READ_ONLY, the_texture.target(), 0,
//                                the_texture.id());
//
//                kernel.setArg(0, img);
//                kernel.setArg(1, m_positionGen);
//                kernel.setArg(2, m_param_buffer);
//
//                int num = num_particles();
//
//                // execute the kernel
//                m_opencl.queue().enqueueNDRangeKernel(kernel,
//                                                      cl::NullRange,
//                                                      cl::NDRange(num),
//                                                      cl::NullRange);
//
//                m_opencl.queue().finish();
//            }
//            catch(cl::Error &error)
//            {
//                LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")";
//            }
//
//        }
//    }
//
    
    void ParticleSystem::apply_forces(float time_delta)
    {
        auto iter = m_kernel_map.find(g_forces_kernel);

        if(iter == m_kernel_map.end()){ LOG_WARNING << "kernel: " << g_forces_kernel << " not found"; }
        else
        {
            // get a ref for our kernel
            auto &force_kernel = iter->second;
            
            try
            {
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

    void ParticleSystem::apply_contraints()
    {
        auto iter = m_kernel_map.find(g_constraints_kernel);

        if(iter == m_kernel_map.end()){ LOG_WARNING << "kernel: " << g_constraints_kernel << " not found"; }
        else
        {
            // get a ref for our kernel
            auto &kernel = iter->second;

            try
            {
                kernel.setArg(0, m_vertices);
                kernel.setArg(1, m_velocities);
                kernel.setArg(2, m_plane_buffer);
                kernel.setArg(3, (int)m_planes.size());
                kernel.setArg(4, m_param_buffer);

                int num = num_particles();

                // execute the kernel
                m_opencl.queue().enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(num), cl::NullRange);
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

    void ParticleSystem::set_lifetime(float the_min, float the_max)
    {
        m_lifetime_min = the_min;
        m_lifetime_max = the_max;
    }

    void ParticleSystem::set_start_velocity(gl::vec3 the_min, gl::vec3 the_max)
    {
        m_start_velocity_min = the_min;
        m_start_velocity_max = the_max;
    }

//    void ParticleSystem::set_param_buffer(void *the_data, size_t num_bytes)
//    {
//        m_opencl.queue().enqueueWriteBuffer(m_param_buffer, CL_TRUE, 0, num_bytes, the_data);
//    }

    void ParticleSystem::update_params()
    {
        // update global param buffer
        Params params;
        params.gravity = vec4(m_gravity, 0);
        params.velocity_min = vec4(m_start_velocity_min, 0);
        params.velocity_max = vec4(m_start_velocity_max, 0);
        params.life_min = m_lifetime_min;
        params.life_max = m_lifetime_max;
        params.bouncyness = m_particle_bounce;
        m_opencl.queue().enqueueWriteBuffer(m_param_buffer, CL_TRUE, 0, sizeof(Params), &params);

        if(!m_planes.empty())
        {
            auto tmp = m_planes;
            gl::mat4 m = glm::inverse(m_mesh->transform());
            std::transform(m_planes.begin(), m_planes.end(), tmp.begin(), [m](const gl::Plane& p)
            {
                return p.transform(m);
            });

            m_opencl.queue().enqueueWriteBuffer(m_plane_buffer, CL_TRUE, 0,
                                                tmp.size() * sizeof(vec4),
                                                tmp.data());
        }

        if(!m_forces.empty())
        {
            auto tmp = m_forces;
            gl::mat4 m = glm::inverse(m_mesh->transform());
            std::transform(m_forces.begin(), m_forces.end(), tmp.begin(), [m](const gl::vec4& f)
            {
                gl::vec4 ret = m * gl::vec4(f.xyz(), 1.f);
                ret.w = f.w;
                return ret;
            });

            m_opencl.queue().enqueueWriteBuffer(m_force_buffer, CL_TRUE, 0,
                                                tmp.size() * sizeof(vec4),
                                                tmp.data());
        }
    }

    void ParticleSystem::set_kernel_source(const std::string &kernel_source)
    {
        m_opencl.set_sources(kernel_source);
        add_kernel(g_update_kernel);
        add_kernel(g_forces_kernel);
        add_kernel(g_constraints_kernel);
    }
}}
