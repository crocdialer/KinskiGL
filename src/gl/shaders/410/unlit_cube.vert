#version 410 core
#extension GL_ARB_separate_shader_objects : enable

uniform mat4 u_modelViewMatrix;
uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;

layout(location = 0) in vec4 a_vertex;

out VertexData
{
    vec3 eyeVec;
} vertex_out;

void main()
{
    vertex_out.eyeVec = a_vertex.xyz;
    gl_Position = u_modelViewProjectionMatrix * a_vertex;
}
