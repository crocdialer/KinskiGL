#version 410 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 a_vertex;

void main()
{
    gl_Position = a_vertex;
}
