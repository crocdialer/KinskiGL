typedef struct User
{
    unsigned int id;
    float3 position;
} User;// 16 byte aligned

/**
 * Hot Iron colormap. It fades between red and yellowish colors.
 *
 * \param value the scaled value
 *
 * \return color.
 */
float4 hotIron(float value)
{
    float4 color8  = (float4)( 255.0 / 255.0, 255.0 / 255.0, 204.0 / 255.0, 1.0 );
    float4 color7  = (float4)( 255.0 / 255.0, 237.0 / 255.0, 160.0 / 255.0, 1.0 );
    float4 color6  = (float4)( 254.0 / 255.0, 217.0 / 255.0, 118.0 / 255.0, 1.0 );
    float4 color5  = (float4)( 254.0 / 255.0, 178.0 / 255.0,  76.0 / 255.0, 1.0 );
    float4 color4  = (float4)( 253.0 / 255.0, 141.0 / 255.0,  60.0 / 255.0, 1.0 );
    float4 color3  = (float4)( 252.0 / 255.0,  78.0 / 255.0,  42.0 / 255.0, 1.0 );
    float4 color2  = (float4)( 227.0 / 255.0,  26.0 / 255.0,  28.0 / 255.0, 1.0 );
    float4 color1  = (float4)( 189.0 / 255.0,   0.0 / 255.0,  38.0 / 255.0, 1.0 );
    float4 color0  = (float4)( 128.0 / 255.0,   0.0 / 255.0,  38.0 / 255.0, 1.0 );
    
    float colorValue = value * 8.0f;
    int sel = (int)( floor( colorValue ) );
    
    if( sel >= 8 )
    {
        return color0;
    }
    else if( sel < 0 )
    {
        return color0;
    }
    else
    {
        colorValue -= (float)( sel );
        
        if( sel < 1 )
        {
            return ( color1 * colorValue + color0 * ( 1.0f - colorValue ) );
        }
        else if( sel < 2 )
        {
            return ( color2 * colorValue + color1 * ( 1.0f - colorValue ) );
        }
        else if( sel < 3 )
        {
            return ( color3 * colorValue + color2 * ( 1.0f - colorValue ) );
        }
        else if( sel < 4 )
        {
            return ( color4 * colorValue + color3 * ( 1.0f - colorValue ) );
        }
        else if( sel < 5 )
        {
            return ( color5 * colorValue + color4 * ( 1.0f - colorValue ) );
        }
        else if( sel < 6 )
        {
            return ( color6 * colorValue + color5 * ( 1.0f - colorValue ) );
        }
        else if( sel < 7 )
        {
            return ( color7 * colorValue + color6 * ( 1.0f - colorValue ) );
        }
        else if( sel < 8 )
        {
            return ( color8 * colorValue + color7 * ( 1.0 - colorValue ) );
        }
        else
        {
            return color0;
        }
    }
}

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
    float strength = 900.0f;
    float3 dir = pos_particle - pos;
    float dist2 = dot(dir, dir);
    dir = normalize(dir);
    return (strength * strength) * dir / dist2;
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
                                 __global float4* velocity,
                                 __read_only image2d_t label_image,
                                 __constant float3* user_positions,
                                 int num_users,
                                 float min_distance,
                                 bool weighted_size,
                                 float force_factor,
                                 float dt)
{
    size_t i = get_global_id(0);
    float3 pos = positions[i];
    float4 color = colors[i];
    float point_size = pointSizes[i];
    float4 vel = velocity[i];
    float heat = 0;
    
    int w = get_image_width(label_image);
    int h = get_image_height(label_image);
    int2 coords = {pos.x + w/2, -pos.y + h/2};
    float4 label = read_imagef(label_image, coords);
    
    float3 cumulative_force = (float3)(0, 0, 0);
    
    for(int j = 0; j < num_users; ++j)
    {
        cumulative_force += create_radial_force(user_positions[j], pos);
        
        float3 diff = user_positions[j] - pos;
        float dist2 = dot(diff, diff);
        
        float min_distance2 = min_distance * min_distance;
        if(dist2 < min_distance2)
        {
            heat += (min_distance2 - dist2) / min_distance2;
        }
    }
    heat = min(heat, .99f);
    cumulative_force.z = 0.f;
    point_size = weighted_size ? point_size * heat : point_size;
    
    // red label
    if(isgreater(label.x, 0.5f) & isless(label.y, 0.5f))
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
//        point_size *= 1.0f - heat;
//        velocity[i] += heat * force_factor * (float4)(0, -1, 0, 0) * dt;
        point_size *= 1.0f + 3.0f * heat;
        color = gray(color);
    }
    // yellow label
    if(isgreater(label.x, 0.5f) & isgreater(label.y, 0.5f))
    {
        color = hotIron(heat);
        //point_size *= 1.0f + 3.0f * heat;
    }
    // debug colormap
    //color = hotIron(heat);
    vel += heat * force_factor * (float4)(cumulative_force, 0) * dt;
    
    // write back our perturbed values
    pointSizes[i] = point_size;
    float4 poo = color + (colors[i] - color) * (1 - heat);
    colors[i] = poo;
    velocity[i] = vel;
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
    
    //apply forces
    //v.xyz += cumulative_force * dt;
    
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
