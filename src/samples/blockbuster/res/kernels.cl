typedef struct Params
{
    float4 gravity, contraints_min, contraints_max;
    float bouncyness;
    
}Params;

inline float4 gray(float4 color)
{
    float y_val = dot(color.xyz, (float3)(0.299, 0.587, 0.114));
    return (float4)(y_val, y_val, y_val, color.w);
}

inline float3 create_radial_force(float3 pos, float3 pos_particle, float strength)
{
    float3 dir = pos_particle - pos;
    float dist2 = dot(dir, dir);
    dir = normalize(dir);
    return strength * dir / dist2;
}

__kernel void texture_input(read_only image2d_t image, __global float3* pos, int num_cols, int num_rows, 
                            float the_min, float the_max, float the_multiplier, float the_smoothing)
{
    unsigned int i = get_global_id(0);

    int w = get_image_width(image);
    int h = get_image_height(image);
    
    int2 array_pos = {w * (i % num_cols) / (float)(num_cols), h * (i / num_cols) / (float)(num_rows)};
    array_pos.y = h - array_pos.y;
    
    float4 color = read_imagef(image, CLK_FILTER_NEAREST | CLK_ADDRESS_CLAMP_TO_EDGE, array_pos);
    //int depth_in_mm = read_imageui(image, CLK_FILTER_NEAREST | CLK_ADDRESS_CLAMP_TO_EDGE, array_pos).x;
    
    // depth value in meters here
    float depth = color.x * 65535.0 / 1000.0;
    
    //float min_val = 1.0, max_val = 2.5;
    //
    float ratio = 0.0;
    if(depth < the_min || depth > the_max){ depth = 0; }
    else
    {
        ratio = (depth - the_min) / (the_max - the_min); 
    
    }
    
    //float outval = (depth < 3.0 && depth > 2.0) ? depth * 20.0 : 0;
    float outval = ratio * the_multiplier;
    pos[i].z = mix(pos[i].z, outval, the_smoothing);
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

    //store the updated life in the velocity array
    v.w = life;
    
    //update the arrays with our newly computed values
    pos[i] = p;
    vel[i] = v;
}
