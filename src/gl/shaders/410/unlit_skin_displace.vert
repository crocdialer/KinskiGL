#version 410 core
#extension GL_ARB_separate_shader_objects : enable

#define DIFFUSE 0
#define DISPLACE 1

struct matrix_struct_t
{
    mat4 model_view;
    mat4 model_view_projection;
    mat4 texture_matrix;
    mat3 normal_matrix;
};

layout(std140) uniform MatrixBlock
{
    matrix_struct_t ubo;
};
uniform mat4 u_bones[110];

// vertex displacement
uniform sampler2D u_sampler_2D[2];
uniform float u_displace_factor = 0.f;

layout(location = 0) in vec4 a_vertex;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_texCoord;
layout(location = 3) in vec4 a_color;
layout(location = 6) in ivec4 a_boneIds;
layout(location = 7) in vec4 a_boneWeights;

out VertexData
{
  vec4 color;
  vec2 texCoord;
} vertex_out;

void main()
{
  vertex_out.color = a_color;
  vertex_out.texCoord = (ubo.texture_matrix * a_texCoord).xy;
  vec4 newVertex = vec4(0);
  vec3 newNormal = vec3(0);

  for (int i = 0; i < 4; i++)
  {
    newVertex += u_bones[a_boneIds[i]] * a_vertex * a_boneWeights[i];
    newNormal += (u_bones[a_boneIds[i]] * vec4(a_normal, 0.0) * a_boneWeights[i]).xyz;
  }
  newVertex = vec4(newVertex.xyz, 1.0);

  float displace = (2.0 * texture(u_sampler_2D[DISPLACE], vertex_out.texCoord.st).x) - 1.0;
  vec4 displace_vert = newVertex + vec4(newNormal * u_displace_factor * displace, 0.f);
  gl_Position = ubo.model_view_projection * displace_vert;
}
