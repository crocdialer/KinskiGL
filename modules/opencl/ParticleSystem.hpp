//
//  ParticleSystem.h
//  gl
//
//  Created by Fabian on 18/07/14.
//
//

#pragma once

#include "gl/Object3D.hpp"
#include "gl/geometry_types.hpp"
#include "opencl/cl_context.h"

namespace kinski{ namespace gl{

DEFINE_CLASS_PTR(ParticleSystem);

KINSKI_API class ParticleSystem : public Object3D
{
public:

    typedef std::map<std::string, cl::Kernel> KernelMap;

    static ParticleSystemPtr create(const cl_context& context = cl_context());

    void update(float time_delta) override;

    void set_mesh(gl::MeshPtr the_mesh);
    gl::MeshPtr mesh() const {return m_mesh;};

    int num_particles() const;

    cl_context& opencl(){return m_opencl;};

    void add_kernel(const std::string &kernel_name);

    void set_kernel_source(const std::string &kernel_source);

    std::vector<gl::Plane>& planes(){ return m_planes; }
    const std::vector<gl::Plane>& planes() const { return m_planes;}

    std::vector<glm::vec4>& forces(){ return m_forces;}
    const std::vector<glm::vec4>& forces() const { return m_forces;}

    void set_gravity(const glm::vec3 &the_gravity);
    const glm::vec3& gravity() const;

    void set_bouncyness(float b){m_particle_bounce = b;};
    float bouncyness() const{return m_particle_bounce;};

    bool use_constraints() const {return m_use_constraints;}
    void set_use_constraints(bool b) {m_use_constraints = b;}

    void set_lifetime(float the_min, float the_max);
    void set_start_velocity(gl::vec3 the_min, gl::vec3 the_max);

    void set_debug_life(bool b){ m_debug_life = b; };

    cl::BufferGL& vertices() { return m_vertices; }
    cl::BufferGL& colors() { return m_colors; }
    cl::BufferGL& normals() { return m_normals; }
    cl::BufferGL& tex_coords() { return m_texCoords; }
    cl::BufferGL& point_sizes() { return m_pointSizes; }
        
    private:

    ParticleSystem();
    ParticleSystem(const cl_context& context);

    void update_params();
    void apply_forces(float time_delta);
    void apply_contraints();

    gl::MeshPtr m_mesh;

    //! holds opencl standard assets
    cl_context m_opencl;

    //! maps kernel names with their actual objects
    KernelMap m_kernel_map;

    // particle system related
    cl::Buffer m_velocities, m_positionGen, m_velocityGen, m_force_buffer, m_param_buffer, m_plane_buffer;

    glm::vec3 m_gravity;

    glm::vec3 m_start_velocity_min, m_start_velocity_max;

    float m_lifetime_min, m_lifetime_max;
    bool m_debug_life;

    float m_particle_bounce;

    //! radial forces -> (x, y, z, strength)
    std::vector<glm::vec4> m_forces;

    //! plane constraints
    std::vector<gl::Plane> m_planes;

    //! constrain particle positions
    bool m_use_constraints;

    // OpenCL buffer objects, corrensponding to the Buffers present in a gl::Mesh instance
    cl::BufferGL m_vertices, m_colors, m_normals, m_texCoords, m_pointSizes;
    cl::ImageGL m_cl_image;
};
    
}}