#version 410

layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 u_view_matrix[6];
uniform mat4 u_projection_matrix;

out VertexData
{
  vec3 eyeVec;
} vertex_out;

void main()
{
    for(int j = 0; j < 6; ++j)
    {
        for(int i = 0; i < 3; ++i)
        {
            vertex_out.eyeVec = (gl_in[i].gl_Position).xyz;
            gl_Position = u_projection_matrix * u_view_matrix[j] * gl_in[i].gl_Position;
            gl_Layer = j;
            EmitVertex();
        }
        EndPrimitive();
    }
}
