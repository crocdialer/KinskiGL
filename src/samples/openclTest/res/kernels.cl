__kernel void set_colors_from_image(image2d_t image, __global float3* pos, __global float4* color)
{
    unsigned int i = get_global_id(0);
    int w = get_image_width(image);
    int h = get_image_height(image);
    
    int2 coords = {pos[i].x + w/2, pos[i].z + h/2};
    color[i] = read_imagef(image, CLK_FILTER_NEAREST | CLK_ADDRESS_CLAMP_TO_EDGE, coords);
}

__kernel void updateParticles(__global float3* pos, __global float4* color, __global float4* vel,
                    __global float4* pos_gen, __global float4* vel_gen, float dt)
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
    
    //apply forces
    //TODO: implement
    v.y -= 2.f * dt;
    
    //update the position with the new velocity
    p.xyz += v.xyz * dt;
    if(p.y < 0) p.y = 0;
    
    //apply contraints
    //TODO: implement
    
    //store the updated life in the velocity array
    v.w = life;
    
    //update the arrays with our newly computed values
    pos[i] = p;
    vel[i] = v;
}
