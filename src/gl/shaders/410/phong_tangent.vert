#version 410

uniform mat4 u_modelViewMatrix;
uniform mat4 u_modelViewProjectionMatrix;
uniform mat3 u_normalMatrix;
uniform mat4 u_textureMatrix;

layout(location = 0) in vec4 a_vertex;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_texCoord;
// layout(location = 3) in vec4 a_color;
layout(location = 5) in vec3 a_tangent;

out VertexData
{
  // vec4 color;
  vec4 texCoord;
  vec3 normal;
  vec3 eyeVec;
  vec3 tangent;
} vertex_out;

void main()
{
  // vertex_out.color = a_color;
  vertex_out.normal = normalize(u_normalMatrix * a_normal);
  vertex_out.tangent = normalize(u_normalMatrix * a_tangent);
  vertex_out.texCoord = u_textureMatrix * a_texCoord;
  vertex_out.eyeVec = (u_modelViewMatrix * a_vertex).xyz;
  gl_Position = u_modelViewProjectionMatrix * a_vertex;
}
