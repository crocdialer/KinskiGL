struct Ball
{
    float3 position;
};

inline float3 create_radial_force(float3 pos, float3 pos_particle)
{
    float strength = 10000000.0;
    float3 dir = pos_particle - pos;
    float dist2 = dot(dir, dir);
    dir = normalize(dir);
    return strength * dir / dist2;
}

__kernel void set_colors_from_image(image2d_t image, __global float3* pos, __global float4* color)
{
    size_t i = get_global_id(0);
    int w = get_image_width(image);
    int h = get_image_height(image);
    
    int2 coords = {pos[i].x + w/2, -pos[i].y + h/2};
    color[i] = read_imagef(image, coords);
}

__kernel void updateParticles(__global float3* pos, __global float4* color, __global float4* vel,
                              __global float4* pos_gen, __global float4* vel_gen, float dt,
                              __global float3* user_positions, int num_users)
{
    //get our index in the array
    size_t i = get_global_id(0);
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
    
    float3 cumulative_force = (float3)(0, 0, 0);
    //apply forces
    for(int j = 0; j < num_users; ++j)
    {
        cumulative_force += - create_radial_force(user_positions[j], p);
    }
    
    v.xyz += cumulative_force * dt;
    
    //TODO: implement
    //v.y -= 2.f * dt;
    
    //update the position with the new velocity
    p += v.xyz * dt;

    //apply contraints
    //TODO: implement
    if(p.z < -10.0) p.z = -10.0;
    
    //store the updated life in the velocity array
    v.w = life;
    
    //update the arrays with our newly computed values
    pos[i] = p;
    vel[i] = v;
    
    //you can manipulate the color based on properties of the system
    //here we adjust the alpha
    
    //color[i] = (float4)(1.f, 1.f, 0.f, 1.f);//life / 5.0;
}
