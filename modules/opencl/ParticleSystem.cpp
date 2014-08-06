//
//  ParticleSystem.cpp
//  kinskiGL
//
//  Created by Fabian on 18/07/14.
//
//

#include "ParticleSystem.hpp"
#include "kinskiGL/Mesh.h"

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
            m_contraints_aabb = geom->boundingBox();
            
            try
            {
                // shared position buffer for OpenGL / OpenCL
                if(!geom->vertices().empty())
                {                
                    m_vertices = cl::BufferGL(m_opencl.context(), CL_MEM_READ_WRITE,
                                              geom->vertexBuffer().id());
                }
                
                if(geom->hasColors())
                {
                    m_colors = cl::BufferGL(m_opencl.context(), CL_MEM_READ_WRITE,
                                            geom->colorBuffer().id());
                }
                if(geom->hasNormals())
                {
                    m_normals = cl::BufferGL(m_opencl.context(), CL_MEM_READ_WRITE,
                                             geom->normalBuffer().id());
                }
                if(geom->hasTexCoords())
                {
                    m_texCoords = cl::BufferGL(m_opencl.context(), CL_MEM_READ_WRITE,
                                               geom->texCoordBuffer().id());
                }
                if(geom->hasPointSizes())
                {
                    m_pointSizes = cl::BufferGL(m_opencl.context(), CL_MEM_READ_WRITE,
                                                geom->pointSizeBuffer().id());
                }
                
                //////////////// create the OpenCL only arrays //////////////////
                
                // combined velocity / life array
                m_velocities = cl::Buffer(m_opencl.context(), CL_MEM_WRITE_ONLY,
                                          geom->vertexBuffer().numBytes());
                
                // spawn positions
                m_positionGen = cl::Buffer(m_opencl.context(), CL_MEM_WRITE_ONLY,
                                           geom->vertexBuffer().numBytes() );
                
                
                m_velocityGen = cl::Buffer(m_opencl.context(), CL_MEM_WRITE_ONLY,
                                           geom->vertexBuffer().numBytes());
                
                
                // reserve memory for 200 forces, seems enough
                m_force_buffer = cl::Buffer(m_opencl.context(), CL_MEM_WRITE_ONLY,
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
                int num_bytes = geom->vertexBuffer().numBytes();
                
                m_opencl.queue().enqueueWriteBuffer(m_velocities, CL_TRUE, 0, num_bytes, &velGen[0]);
                m_opencl.queue().enqueueWriteBuffer(m_velocityGen, CL_TRUE, 0, num_bytes, &velGen[0]);
                
                // generate spawn positions from original positions
                uint8_t *vert_buf = geom->vertexBuffer().map();
                m_opencl.queue().enqueueWriteBuffer(m_positionGen, CL_TRUE, 0,
                                                    geom->vertexBuffer().numBytes(),
                                                    vert_buf);
                geom->vertexBuffer().unmap();
            }
            catch(cl::Error &error)
            {
                LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")";
            }
        }
    
    }
    
    void ParticleSystem::update(float time_delta)
    {
        // update global param buffer
        Params params;
        params.gravity = vec4(m_gravity, 0);
        
        params.contraints_min = m_mesh->transform() * (m_use_constraints ? vec4(m_contraints_aabb.min, 1) :
            vec4(std::numeric_limits<float>::lowest()));
        
        params.contraints_max = m_mesh->transform() * (m_use_constraints ? vec4(m_contraints_aabb.max, 1) :
            vec4(std::numeric_limits<float>::max()));
        params.bouncyness = m_particle_bounce;
        
        m_opencl.queue().enqueueWriteBuffer(m_param_buffer, CL_TRUE, 0, sizeof(Params), &params,
                                            NULL);
        
        // apply our forces
        apply_forces(time_delta);
        
        auto iter = m_kernel_map.find("updateParticles");
        if(iter == m_kernel_map.end())
        {
            LOG_WARNING << "no particle kernel found";
            return;
        }
        
        // get a ref for our kernel
        auto &kernel = iter->second;
        
        try
        {
            vector<cl::Memory> glBuffers = {m_vertices, m_colors};
            
            // Make sure OpenGL is done using our VBOs
            glFinish();
            
            // map OpenGL buffer object for writing from OpenCL
            // this passes in the vector of VBO buffer objects (position and color)
            m_opencl.queue().enqueueAcquireGLObjects(&glBuffers);
            
            kernel.setArg(0, m_vertices);
            kernel.setArg(1, m_colors);
            kernel.setArg(2, m_velocities);
            kernel.setArg(3, m_positionGen);
            kernel.setArg(4, m_velocityGen);
            kernel.setArg(5, time_delta); //pass in the timestep
            kernel.setArg(6, m_param_buffer);
            
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
    
    void ParticleSystem::apply_forces(float time_delta)
    {
        auto iter = m_kernel_map.find("apply_forces");
        if(iter == m_kernel_map.end())
        {
            LOG_WARNING << "no apply_forces kernel found";
            return;
        }
        
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
}}
