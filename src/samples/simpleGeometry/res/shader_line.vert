#version 150 core
uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;

in vec4 a_vertex;
in vec4 a_texCoord;
in vec4 a_color;

out VertexData {
    vec4 color;
    vec4 texCoord;
} vertex_out;

void main()
{
    vertex_out.color = a_color;
    vertex_out.texCoord =  u_textureMatrix * a_texCoord;
    gl_Position = u_modelViewProjectionMatrix * a_vertex;
}