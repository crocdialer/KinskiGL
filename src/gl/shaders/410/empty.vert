#version 410 core

uniform mat4 u_modelViewProjectionMatrix;
// uniform mat4 u_modelViewMatrix;

layout(location = 0) in vec4 a_vertex;

void main()
{
    // gl_Position = u_modelViewMatrix * a_vertex;
    gl_Position = u_modelViewProjectionMatrix * a_vertex;
    // gl_Position = a_vertex;
}
