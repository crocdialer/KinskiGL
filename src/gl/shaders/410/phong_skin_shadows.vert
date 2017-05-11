#version 410

#define NUM_SHADOW_LIGHTS 2

uniform mat4 u_modelViewMatrix;
uniform mat4 u_modelViewProjectionMatrix;
uniform mat3 u_normalMatrix;
uniform mat4 u_shadow_matrices[NUM_SHADOW_LIGHTS];
uniform mat4 u_textureMatrix;
uniform mat4 u_bones[110];

layout(location = 0) in vec4 a_vertex;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_texCoord;
layout(location = 3) in vec4 a_color;
layout(location = 6) in ivec4 a_boneIds;
layout(location = 7) in vec4 a_boneWeights;

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

  vec4 newVertex = vec4(0);
  vec4 newNormal = vec4(0);

  for (int i = 0; i < 4; i++)
  {
    newVertex += u_bones[a_boneIds[i]] * a_vertex * a_boneWeights[i];
    newNormal += u_bones[a_boneIds[i]] * vec4(a_normal, 0.0) * a_boneWeights[i];
  }
  newVertex = vec4(newVertex.xyz, 1.0);
  vertex_out.normal = normalize(u_normalMatrix * newNormal.xyz);
  vertex_out.texCoord = u_textureMatrix * a_texCoord;
  vertex_out.eyeVec = (u_modelViewMatrix * newVertex).xyz;
  for(int i = 0; i < NUM_SHADOW_LIGHTS; i++)
  {
    vertex_out.lightspace_pos[i] = u_shadow_matrices[i] * newVertex;
  }
  gl_Position = u_modelViewProjectionMatrix * newVertex;
}