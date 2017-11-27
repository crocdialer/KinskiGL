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
        const std::string g_spawn_kernel = "spawn_particle";
        const std::string g_kill_kernel = "kill_particles";
    }

    struct Params
    {
        vec4 emitter_position;
        vec4 gravity;
        vec4 velocity_min, velocity_max;
        mat3 rotation_matrix;
        float bouncyness;
        float life_min, life_max;
        bool debug_life;
        uint32_t num_alive;
    };

    ParticleSystemPtr ParticleSystem::create(const cl_context& context)
    {
        return ParticleSystemPtr(new ParticleSystem(context));
    }

    ParticleSystem::ParticleSystem(): ParticleSystem(cl_context())
    {
        
    }
    
    ParticleSystem::ParticleSystem(const cl_context& context):
    m_opencl(context),
    m_gravity(vec3(0, -9.87f, 0)),
    m_start_velocity_min(0),
    m_start_velocity_max(0),
    m_emission_rate(1000.f),
    m_emission_accum(0.f),
    m_lifetime_min(1.f),
    m_lifetime_max(3.f),
    m_debug_life(false),
    m_particle_bounce(0.f),
    m_use_constraints(true)
    {
    
    }

    void ParticleSystem::init_with_count(size_t the_particle_count)
    {
        auto geom = gl::Geometry::create();
        auto mat = gl::Material::create(gl::ShaderType::POINTS_COLOR);
        geom->set_primitive_type(GL_POINTS);
        geom->vertices().resize(the_particle_count, vec3(0));
        geom->colors().resize(the_particle_count, gl::COLOR_WHITE);
        geom->point_sizes().resize(the_particle_count, 1.f);
        auto &indices = geom->indices();
        indices.resize(the_particle_count);
        for(uint32_t i = 0; i < the_particle_count; ++i){ indices[i] = i; }
        mat->set_point_size(1.f);
        mat->set_point_attenuation(0.f, 0.01f, 0.f);
        mat->uniform("u_pointRadius", 50.f);
        mat->set_point_size(1.f);
        mat->set_blending();
        set_mesh(gl::Mesh::create(geom, mat));
    }

    void ParticleSystem::set_mesh(gl::MeshPtr the_mesh)
    {
        if(!m_mesh){ m_opencl.init(); }
        else{ remove_child(m_mesh); }

        m_mesh = the_mesh;
        add_child(m_mesh);

        if(m_mesh)
        {
            m_mesh->geometry()->create_gl_buffers(GL_STREAM_DRAW);
            gl::GeometryConstPtr geom = m_mesh->geometry();
            
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
                if(geom->has_indices())
                {
                    m_indices = cl::BufferGL(m_opencl.context(), CL_MEM_READ_WRITE,
                                             geom->index_buffer().id());
                }
                m_mesh->create_vertex_attribs();

//                vector<glm::vec4> velGen(num_particles(), gl::vec4(0));
//                for(uint32_t i = 0; i < num_particles(); i++)
//                {
//                    float life = 0.f;//kinski::random(m_lifetime_min, m_lifetime_max);
//                    glm::vec3 vel = glm::vec3(0);//glm::linearRand(m_start_velocity_min, m_start_velocity_max);
//                    velGen.push_back(glm::vec4(vel, life));
//                }

                // create velocity/life VBO and VertexAttrib
                gl::Buffer velocity_vbo(vector<glm::vec4>(geom->vertices().size(), gl::vec4(0)),
                                        GL_ARRAY_BUFFER,
                                        GL_STREAM_DRAW);
                
                gl::Mesh::VertexAttrib va;
                va.name = "a_velocity";
                va.buffer = velocity_vbo;
                va.size = 4;
                m_mesh->add_vertex_attrib(va);

                m_velocities = cl::BufferGL(m_opencl.context(), CL_MEM_READ_WRITE, velocity_vbo.id());

                //////////////// create the OpenCL only arrays //////////////////

                // spawn positions
                m_positionGen = cl::Buffer(m_opencl.context(), CL_MEM_READ_WRITE,
                                           geom->vertex_buffer().num_bytes() );

                // reserve memory for 200 forces, seems enough
                m_force_buffer = cl::Buffer(m_opencl.context(), CL_MEM_READ_WRITE,
                                            200 * sizeof(glm::vec4));

                m_param_buffer = cl::Buffer(opencl().context(), CL_MEM_READ_WRITE, sizeof(Params), NULL);
                
                m_plane_buffer = cl::Buffer(m_opencl.context(), CL_MEM_READ_WRITE,
                                            200 * sizeof(gl::Plane));
                
                // generate spawn positions from original positions
                const uint8_t *vert_buf = geom->vertex_buffer().map();
                m_opencl.queue().enqueueWriteBuffer(m_positionGen, CL_TRUE, 0,
                                                    geom->vertex_buffer().num_bytes(),
                                                    vert_buf);
                geom->vertex_buffer().unmap();
                
                m_num_alive = 0;
                update_params();
            }
            catch(cl::Error &error)
            {
                LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")";
            }
        }
    
    }

    void ParticleSystem::update(float time_delta)
    {
        Object3D::update(time_delta);

        if(!m_mesh){ return; }

        // make sure the particle mesh has global identity transform
        m_mesh->set_global_transform(gl::mat4());
        
        m_emission_accum = std::min<float>(m_emission_accum + m_emission_rate * time_delta,
                                           max_num_particles() - m_num_alive);
        
        auto iter = m_kernel_map.find(g_update_kernel);

        if(iter == m_kernel_map.end()){ LOG_WARNING << "kernel: " << g_update_kernel << " not found"; }
        else
        {
            // get a ref for our kernel
            auto &kernel = iter->second;
            
            try
            {
                vector<cl::Memory>
                glBuffers = {m_vertices, m_velocities, m_colors, m_pointSizes, m_indices};
                
                // Make sure OpenGL is done using our VBOs
                glFinish();
                
                // map OpenGL buffer object for writing from OpenCL
                // this passes in the vector of VBO buffer objects (position and color)
                m_opencl.queue().enqueueAcquireGLObjects(&glBuffers);
                
                apply_emission();
                
                // pass global parameters to a buffer used by all kernels
                update_params();
                
                uint32_t num = num_particles();
                
                if(num)
                {
                    // apply our forces
                    apply_forces(time_delta);
                    
                    kernel.setArg(0, m_vertices);
                    kernel.setArg(1, m_colors);
                    kernel.setArg(2, m_pointSizes);
                    kernel.setArg(3, m_velocities);
                    kernel.setArg(4, m_positionGen);
                    kernel.setArg(5, m_indices);
                    kernel.setArg(6, time_delta); //pass in the timestep
                    kernel.setArg(7, m_param_buffer);
                    
                    // execute the kernel
                    m_opencl.queue().enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(num), cl::NullRange);
                    
                    // apply our constraints
                    if(m_use_constraints){ apply_contraints(); }
                    
                    apply_killing();
                }
                
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
    
    void ParticleSystem::apply_forces(float time_delta)
    {
        auto iter = m_kernel_map.find(g_forces_kernel);

        if(iter == m_kernel_map.end()){ LOG_WARNING << "kernel: " << g_forces_kernel << " not found"; }
        else
        {
            int num = num_particles();
            
            if(num)
            {
                // get a ref for our kernel
                auto &force_kernel = iter->second;
                
                try
                {
                    force_kernel.setArg(0, m_vertices);
                    force_kernel.setArg(1, m_velocities);
                    force_kernel.setArg(2, m_indices);
                    force_kernel.setArg(3, m_force_buffer);
                    force_kernel.setArg(4, (int)m_forces.size());
                    force_kernel.setArg(5, time_delta);
                    force_kernel.setArg(6, m_param_buffer);
                    
                    // execute the kernel
                    m_opencl.queue().enqueueNDRangeKernel(force_kernel,
                                                          cl::NullRange,
                                                          cl::NDRange(num),
                                                          cl::NullRange);
//                    m_opencl.queue().finish();
                }
                catch(cl::Error &error)
                {
                    LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")";
                }
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
                kernel.setArg(2, m_indices);
                kernel.setArg(3, m_plane_buffer);
                kernel.setArg(4, (int)m_planes.size());
                kernel.setArg(5, m_param_buffer);

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

    uint32_t ParticleSystem::num_particles() const
    {
        return m_num_alive;
    }
    
    uint32_t ParticleSystem::max_num_particles() const
    {
        if(m_mesh){ return m_mesh->geometry()->vertices().size(); }
        return 0;
    }
    
    size_t ParticleSystem::emit_particles(size_t the_num)
    {
        the_num = std::min<size_t>(the_num, max_num_particles() - m_emission_accum);
        m_emission_accum += the_num;
        return the_num;
    }
    
    void ParticleSystem::apply_emission()
    {
        uint32_t num = std::min<uint32_t>(m_emission_accum, max_num_particles() - m_num_alive);
        m_emission_accum -= num;
        
        auto iter = m_kernel_map.find(g_spawn_kernel);
        
        if(iter == m_kernel_map.end()){ LOG_WARNING << "kernel: " << g_spawn_kernel << " not found"; }
        else
        {
            // get a ref for our kernel
            auto &kernel = iter->second;
            
            if(num)
            {
                try
                {
                    kernel.setArg(0, m_vertices);
                    kernel.setArg(1, m_colors);
                    kernel.setArg(2, m_pointSizes);
                    kernel.setArg(3, m_velocities);
                    kernel.setArg(4, m_positionGen);
                    kernel.setArg(5, m_indices);
                    kernel.setArg(6, m_param_buffer);
                    
                    // execute the kernel
                    m_opencl.queue().enqueueNDRangeKernel(kernel,
                                                          cl::NDRange(m_num_alive),
                                                          cl::NDRange(num),
                                                          cl::NullRange);
//                    m_opencl.queue().finish();
                    m_num_alive += num;
                }
                catch(cl::Error &error)
                {
                    LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")";
                }
            }
        }
    }
    
    void ParticleSystem::apply_killing()
    {
        auto iter = m_kernel_map.find(g_kill_kernel);
        
        if(iter == m_kernel_map.end()){ LOG_WARNING << "kernel: " << g_kill_kernel << " not found"; }
        else
        {
            // get a ref for our kernel
            auto &kernel = iter->second;
            
            if(m_num_alive)
            {
                try
                {
                    kernel.setArg(0, m_velocities);
                    kernel.setArg(1, m_indices);
                    kernel.setArg(2, m_param_buffer);
                    
                    // execute the kernel
                    m_opencl.queue().enqueueNDRangeKernel(kernel,
                                                          cl::NullRange,
                                                          cl::NDRange(m_num_alive),
                                                          cl::NullRange);
                    m_opencl.queue().finish();
                    
                    // read back num alive particles
                    Params params;
                    m_opencl.queue().enqueueReadBuffer(m_param_buffer, CL_TRUE, 0, sizeof(Params), &params);
                    m_num_alive = std::max<uint32_t>(params.num_alive, 0);
                    m_mesh->entries().front().num_indices = m_num_alive;
                }
                catch(cl::Error &error)
                {
                    LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")";
                }
            }
        }
    }
    
    float ParticleSystem::emission_rate() const
    {
        return m_emission_rate;
    }
    
    void ParticleSystem::set_emission_rate(float the_rate)
    {
        m_emission_rate = the_rate;
    }
    
    void ParticleSystem::add_kernel(const std::string &kernel_name)
    {
        // OpenCL
        try
        {
            auto pair = std::make_pair(kernel_name, cl::Kernel(opencl().program(), kernel_name.c_str()));
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

    void ParticleSystem::update_params()
    {
        gl::mat3 m(global_transform());

        // update global param buffer
        Params params;
        params.emitter_position = vec4(global_position(), 1.f);
        params.gravity = vec4(m_gravity, 0);
        params.velocity_min = vec4(m_start_velocity_min, 0);
        params.velocity_max = vec4(m_start_velocity_max, 0);
        params.rotation_matrix = global_transform();
        params.life_min = m_lifetime_min;
        params.life_max = m_lifetime_max;
        params.debug_life = m_debug_life;
        params.bouncyness = m_particle_bounce;
        params.num_alive = m_num_alive;
        
        m_opencl.queue().enqueueWriteBuffer(m_param_buffer, CL_TRUE, 0, sizeof(Params), &params);

        if(!m_planes.empty())
        {
            auto tmp = m_planes;
            gl::mat4 m = glm::inverse(m_mesh->global_transform());
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
            gl::mat4 m = glm::inverse(m_mesh->global_transform());
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
        add_kernel(g_spawn_kernel);
        add_kernel(g_kill_kernel);
    }
}}
