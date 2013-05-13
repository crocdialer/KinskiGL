typedef struct User
{
    unsigned int id;
    float3 position;
} User;// 16 byte aligned

float4 gray(float4 color)
{
    float y_val = dot(color.xyz, (float3)(0.299, 0.587, 0.114));
    return (float4)(y_val, y_val, y_val, color.w);
}

inline float4 jet(float val)
{
    return (float4)(min(4.0 * val - 1.5, -4.0 * val + 4.5),
                    min(4.0 * val - 0.5, -4.0 * val + 3.5),
                    min(4.0 * val + 0.5, -4.0 * val + 2.5),
                    1.0);
}

inline float3 create_radial_force(float3 pos, float3 pos_particle)
{
    float strength = 5000000.0;
    float3 dir = pos_particle - pos;
    float dist2 = dot(dir, dir);
    dir = normalize(dir);
    return strength * dir / dist2;
}

__kernel void set_colors_from_image(__read_only image2d_t image, __global const float3* pos, __global float4* color)
{
    size_t i = get_global_id(0);
    int w = get_image_width(image);
    int h = get_image_height(image);
    
    int2 coords = {pos[i].x + w/2, -pos[i].y + h/2};
    color[i] = read_imagef(image, coords);
}

__kernel void process_user_input(__global float3* positions,/*VBO*/
                                 __global float4* colors,/*VBO*/
                                 __global float* pointSizes /*VBO*/,
                                 __read_only image2d_t label_image,
                                 __constant float3* user_positions,
                                 int num_users,
                                 float min_distance)
{
    size_t i = get_global_id(0);
    float3 pos = positions[i];
    float4 color = colors[i];
    float point_size = pointSizes[i];
    float heat = 0;
    
    int w = get_image_width(label_image);
    int h = get_image_height(label_image);
    int2 coords = {pos.x + w/2, -pos.y + h/2};
    float4 label = read_imagef(label_image, coords);
    
    for(int j = 0; j < num_users; ++j)
    {
        //cumulative_force += create_radial_force(user_positions[j], p);
        float3 diff = user_positions[j] - pos;
        float dist2 = dot(diff, diff);
        
        float min_distance2 = min_distance * min_distance;
        if(dist2 < min_distance2)
        {
            heat += (min_distance2 - dist2) / min_distance2;
        }
    }
    heat = min(heat, 1.0f);
    
    // red label
    if(isgreater(label.x, 0.5f))
    {
        point_size *= 1.0f + 3.0f * heat;
    }
    // green label
    if(isgreater(label.y, 0.5f))
    {
        color = jet(heat);
    }
    // blue label
    if(isgreater(label.z, 0.5f))
    {
        point_size *= 1.0f - heat;
    }
    // yellow label
    if(isgreater(label.x, 0.5f) & isgreater(label.y, 0.5f))
    {
        
    }
    // debug colormap
    //color = jet(heat);
    
    // write back our perturbed values
    pointSizes[i] = point_size;
    float4 poo = color + (colors[i] - color) * (1 - heat);
    colors[i] = poo;
}

__kernel void updateParticles(__global float3* pos, // VBO
                              __global float4* colors, // VBO
                              __global float* pointSizes, // VBO
                              __global float4* vel,
                              __global const float4* pos_gen,
                              __global const float4* vel_gen,
                              __global const float* pointSize_gen,
                              float dt,
                              __constant float3* user_positions,
                              int num_users)
{
    //__local float values[GROUP_SIZE];
    
    //get our index in the array
    size_t i = get_global_id(0);
    //copy position and velocity for this iteration to a local variable
    //note: if we were doing many more calculations we would want to have opencl
    //copy to a local memory array to speed up memory access
    float3 p = pos[i];
    float4 v = vel[i];
    float point_size = pointSize_gen[i];
    
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
    
    float3 cumulative_force = (float3)(0, 0, 0);
    
    //apply forces
    v.xyz += cumulative_force * dt;
    
    //TODO: implement
    //v.y -= 2.f * dt;
    
    //update the position with the new velocity
    p += v.xyz * dt;

    //apply contraints
    //TODO: implement
    if(p.z < -10.0 || p.z > 10.0) p.z = 0.0;
    
    //store the updated life in the velocity array
    v.w = life;
    
    //update the arrays
    pos[i] = p;
    vel[i] = v;
    pointSizes[i] = point_size;
}
