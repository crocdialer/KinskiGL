#version 410 core

layout(location = 0) in vec4 a_vertex;

void main()
{
    gl_Position = a_vertex;
}
