#version 410 core
#extension GL_ARB_separate_shader_objects : enable

uniform mat4 u_modelViewProjectionMatrix;

layout(location = 0) in vec4 a_vertex;

void main()
{
    gl_Position = u_modelViewProjectionMatrix * a_vertex;
}
