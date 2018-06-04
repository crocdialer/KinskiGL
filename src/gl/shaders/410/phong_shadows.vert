#version 410 core
#extension GL_ARB_separate_shader_objects : enable

#define NUM_SHADOW_LIGHTS 4

uniform mat4 u_modelViewMatrix;
uniform mat4 u_modelViewProjectionMatrix;
uniform mat3 u_normalMatrix;
uniform mat4 u_shadow_matrices[NUM_SHADOW_LIGHTS];
uniform mat4 u_textureMatrix;

layout(location = 0) in vec4 a_vertex;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_texCoord;
layout(location = 3) in vec4 a_color;

out VertexData
{
  vec4 color;
  vec4 texCoord;
  vec3 normal;
  vec3 eyeVec;
  vec4 lightspace_pos[NUM_SHADOW_LIGHTS];
} vertex_out;

void main()
{
  vertex_out.color = a_color;
  vertex_out.normal = normalize(u_normalMatrix * a_normal);
  vertex_out.texCoord = u_textureMatrix * a_texCoord;
  vertex_out.eyeVec = (u_modelViewMatrix * a_vertex).xyz;
  for(int i = 0; i < NUM_SHADOW_LIGHTS; i++)
  {
    vertex_out.lightspace_pos[i] = u_shadow_matrices[i] * a_vertex;
  }
  gl_Position = u_modelViewProjectionMatrix * a_vertex;
}
