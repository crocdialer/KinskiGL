#version 410

uniform mat4 u_modelViewMatrix;
uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;

layout(location = 0) in vec4 a_vertex;
layout(location = 2) in vec4 a_texCoord;
layout(location = 3) in vec4 a_color;

out VertexData
{
  vec4 color;
  vec2 texCoord;
  vec3 eyeVec;
} vertex_out;

void main()
{
  vertex_out.color = a_color;
  vertex_out.texCoord = (u_textureMatrix * a_texCoord).xy;
  vertex_out.eyeVec = a_vertex.xyz;
  gl_Position = u_modelViewProjectionMatrix * a_vertex;
}
