#version 410

uniform mat4 u_modelViewMatrix;
uniform mat4 u_modelViewProjectionMatrix;

layout(location = 0) in vec4 a_vertex;

out VertexData
{
  vec3 position;
} vertex_out;

void main()
{
    vertex_out.position = (u_modelViewMatrix * a_vertex).xyz;
    gl_Position = u_modelViewProjectionMatrix * a_vertex;
}
