typedef struct Params
{
    float4 gravity, contraints_min, contraints_max;
    float bouncyness;
    
}Params;

inline float3 create_radial_force(float3 pos, float3 pos_particle, float strength)
{
    float3 dir = pos_particle - pos;
    float dist2 = dot(dir, dir);
    dir = normalize(dir);
    return strength * dir / dist2;
}

__kernel void set_colors_from_image(image2d_t image, __global float3* pos, __global float4* color)
{
    unsigned int i = get_global_id(0);
    int w = get_image_width(image);
    int h = get_image_height(image);
    
    int2 coords = {pos[i].x + w/2, pos[i].z + h/2};
    color[i] = read_imagef(image, CLK_FILTER_NEAREST | CLK_ADDRESS_CLAMP_TO_EDGE, coords);
}

// apply forces and change velocities
__kernel void apply_forces( __global float3* pos,
                            __global float4* vel,
                            __constant float4* force_positions,
                            int num_forces,
                            float dt,
                            __constant struct Params *params)
{
    //get our index in the array
    unsigned int i = get_global_id(0);
    
    float3 p = pos[i];
    
    // add up all forces
    float3 cumulative_force = params->gravity.xyz;

    for(int j = 0; j < num_forces; ++j)
    {
        float3 force_pos = force_positions[j].xyz;
        float force_strength = force_positions[j].w;
        float3 force = create_radial_force(force_pos, p, force_strength);
        
        // force always downwards
        force.y *= cumulative_force.y < 0;

        cumulative_force += force;
    }
    // change velocity here
    vel[i] += (float4)(cumulative_force, 0) * dt;
}

__kernel void updateParticles(  __global float3* pos,
                                __global float4* color,
                                __global float4* vel,
                                __global float4* pos_gen,
                                __global float4* vel_gen,
                                float dt,
                                __constant struct Params *params)
{
    //get our index in the array
    unsigned int i = get_global_id(0);
    
    //copy position and velocity for this iteration to a local variable
    //note: if we were doing many more calculations we would want to have opencl
    //copy to a local memory array to speed up memory access (this will be the subject of a later tutorial)
    float3 p = pos[i];
    float4 v = vel[i];
    
    //we've stored the life in the fourth component of our velocity array
    float life = vel[i].w;
    
    //decrease the life by the time step (this value could be adjusted to lengthen or shorten particle life
    life -= dt;
    
    //if the life is 0 or less we reset the particle's values back to the original values and set life to 1
    if(life <= 0)
    {
        p = pos_gen[i].xyz;
        v = vel_gen[i];
        life = vel_gen[i].w;
    }

    //update the position with the new velocity
    p.xyz += v.xyz * dt;

    //apply contraints
    int3 min_hit = p < params->contraints_min.xyz;
    int3 max_hit = p > params->contraints_max.xyz;
    int3 combo_hit = min_hit | max_hit;
    
    // ground hit
//    if(p.y < 0)
//    {
//        p.y = 0;
//        v.y *= -.4;
//    }

    p = min_hit ? params->contraints_min.xyz : p;
    p = max_hit ? params->contraints_max.xyz : p;
    
    // bounce back at boundaries
    v.xyz = combo_hit ? -params->bouncyness * v.xyz : v.xyz;

    //store the updated life in the velocity array
    v.w = life;
    
    //update the arrays with our newly computed values
    pos[i] = p;
    vel[i] = v;
}
