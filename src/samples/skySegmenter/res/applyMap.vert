#version 150 core

uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;

in vec4 a_vertex;
in vec4 a_texCoord;

out vec4 v_texCoord;

void main()
{
    v_texCoord =  u_textureMatrix * a_texCoord;
    gl_Position = u_modelViewProjectionMatrix * a_vertex;
}

