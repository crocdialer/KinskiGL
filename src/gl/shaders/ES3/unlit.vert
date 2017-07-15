#version 310 es

uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;

layout(location = 0) in vec4 a_vertex;
layout(location = 2) in vec4 a_texCoord;
layout(location = 3) in vec4 a_color;

out vec4 v_color;
out vec2 v_texCoord;

void main()
{
  v_color = a_color;
  v_texCoord = (u_textureMatrix * a_texCoord).xy;
  gl_Position = u_modelViewProjectionMatrix * a_vertex;
}
