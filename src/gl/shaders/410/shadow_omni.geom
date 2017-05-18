#version 410

layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 u_shadow_matrices[6];

// out vec4 FragPos; // FragPos from GS (output per emitvertex)

void main()
{
    for(gl_Layer = 0; gl_Layer < 6; ++gl_Layer)
    {
        for(int i = 0; i < 3; ++i)
        {
            // FragPos = gl[i].gl_Position;
            gl_Position = u_shadow_matrices[gl_Layer] * gl_in[i].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    }
}
