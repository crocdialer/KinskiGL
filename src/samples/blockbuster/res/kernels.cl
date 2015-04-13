typedef struct Params
{
    int num_cols, num_rows, mirror;
    float depth_min, depth_max, multiplier;
    float smooth_fall, smooth_rise;
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

__kernel void texture_input(read_only image2d_t depth_img, __global float4* pos_gen, __constant struct Params *p)
{
    unsigned int i = get_global_id(0);

    //borders
    int row = i / p->num_cols;
    int col = i % p->num_cols;

    if(row == 0 || col == 0 || col == p->num_cols - 1)
    {
      pos_gen[i].z = 0.f; 
      return;
    }

    int depth_img_w = get_image_width(depth_img);
    int depth_img_h = get_image_height(depth_img);
    
    int2 array_pos = {depth_img_w * (col / (float)(p->num_cols)),
                      depth_img_h * (row / (float)(p->num_rows))};
    array_pos.y = depth_img_h - array_pos.y - 1;
    array_pos.x = p->mirror ? depth_img_w - array_pos.x - 1 : array_pos.x; 
    
    // depth value in meters here
    float depth = read_imagef(depth_img, CLK_FILTER_NEAREST | CLK_ADDRESS_CLAMP_TO_EDGE, array_pos).x * 65535.f / 1000.f;
    
    //
    float ratio = 0.f;
    if(depth < p->depth_min || depth > p->depth_max){ depth = 0; }
    else
    {
        ratio = (depth - p->depth_min) / (p->depth_max - p->depth_min);
        ratio = 1.f - ratio;//p->multiplier < 0.f ? 1 - ratio : ratio; 
    }
    float outval = ratio * p->multiplier;
    pos_gen[i].z = outval;
}

__kernel void updateParticles(  __global float4* pos,
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
    float4 p = pos[i];
    float4 v = vel[i];
    
    //we've stored the life in the fourth component of our velocity array
    float life = vel[i].w;
    
    //decrease the life by the time step (this value could be adjusted to lengthen or shorten particle life
    //life -= dt;
    
    //if the life is 0 or less we reset the particle's values back to the original values and set life to 1
    //if(life <= 0)
    //{
    //    p = pos_gen[i].xyz;
    //    v = vel_gen[i];
    //    life = vel_gen[i].w;
    //}

    //update the position with the new velocity
    //p.xyz += v.xyz * dt;
    float4 target_pos = pos_gen[i];
    p = p.z > target_pos.z ? mix(target_pos, p, params->smooth_fall) : mix(target_pos, p, params->smooth_rise);

    //store the updated life in the velocity array
    v.w = life;
    
    //update the arrays with our newly computed values
    pos[i] = p;
    vel[i] = v;
}
