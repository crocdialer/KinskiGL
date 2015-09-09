//
//  ParticleSystem.h
//  gl
//
//  Created by Fabian on 18/07/14.
//
//

#ifndef __gl__ParticleSystem__
#define __gl__ParticleSystem__

#include "gl/KinskiGL.h"
#include "gl/geometry_types.h"
#include "opencl/cl_context.h"

namespace kinski{ namespace gl{

    typedef std::shared_ptr<class ParticleSystem> ParticleSystemPtr;
    
    KINSKI_API class ParticleSystem
    {
    public:
        
        typedef std::map<std::string, cl::Kernel> KernelMap;
        
        ParticleSystem();
        ParticleSystem(const cl_context& context);
        
        void update(float time_delta);
        
        void set_mesh(gl::MeshPtr the_mesh);
        gl::MeshPtr mesh() const {return m_mesh;};
        
        int num_particles() const;
        
        cl_context& opencl(){return m_opencl;};
        
        void add_kernel(const std::string &kernel_name);
        
        std::vector<glm::vec4>& forces(){ return m_forces;}
        const std::vector<glm::vec4>& forces() const { return m_forces;}
        
        void set_gravity(const glm::vec3 &the_gravity);
        const glm::vec3& gravity() const;
        
        void set_bouncyness(float b){m_particle_bounce = b;};
        float bouncyness() const{return m_particle_bounce;};
        
        bool use_constraints() const {return m_use_constraints;}
        void set_use_constraints(bool b) {m_use_constraints = b;}
        
        void set_aabb(gl::AABB the_aabb){m_contraints_aabb = the_aabb;}
        
        void texture_input(gl::Texture &the_texture);
        
        // hacky ->remove when tex input is generic for multiple textures
        void texture_input_alt(gl::Texture &the_tex1, gl::Texture &the_tex2);
        
        void set_param_buffer(void *the_data, size_t num_bytes);
        
        cl::BufferGL& vertices() { return m_vertices; }
        cl::BufferGL& colors() { return m_colors; }
        cl::BufferGL& normals() { return m_normals; }
        cl::BufferGL& tex_coords() { return m_texCoords; }
        cl::BufferGL& point_sizes() { return m_pointSizes; }
        
    private:
        
        void apply_forces(float time_delta);
        
        gl::MeshPtr m_mesh;
        
        //! holds opencl standard assets
        cl_context m_opencl;
        
        //! maps kernel names with their actual objects
        KernelMap m_kernel_map;
        
        // particle system related
        cl::Buffer m_velocities, m_positionGen, m_velocityGen, m_force_buffer, m_param_buffer;
        
        glm::vec3 m_gravity;
        
        float m_particle_bounce;
        
        //! radial forces -> (x, y, z, strength)
        std::vector<glm::vec4> m_forces;
        
        //! an aabb to constrain particle positions
        bool m_use_constraints;
        gl::AABB m_contraints_aabb;
        
        // OpenCL buffer objects, corrensponding to the Buffers present in a gl::Mesh instance
        cl::BufferGL m_vertices, m_colors, m_normals, m_texCoords, m_pointSizes;
        cl::ImageGL m_cl_image;
    };
    
}}

#endif /* defined(__gl__ParticleSystem__) */
