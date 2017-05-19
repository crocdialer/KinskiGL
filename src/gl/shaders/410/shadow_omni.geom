#version 410

layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 u_shadow_view[6];
uniform mat4 u_shadow_projection;

out VertexData
{
  vec3 eyeVec;
} vertex_out;

void main()
{
    for(gl_Layer = 0; gl_Layer < 6; ++gl_Layer)
    {
        for(int i = 0; i < 3; ++i)
        {
            vec4 tmp = u_shadow_view[gl_Layer] * gl_in[i].gl_Position;
            vertex_out.eyeVec = tmp.xyz;
            gl_Position = u_shadow_projection * tmp;
            EmitVertex();
        }
        EndPrimitive();
    }
}
