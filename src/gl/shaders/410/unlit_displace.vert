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

uniform sampler2D u_sampler_2D[2];
uniform float u_displace_factor = 0.f;

layout(location = 0) in vec4 a_vertex;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_texCoord;
layout(location = 3) in vec4 a_color;

out VertexData
{
  vec4 color;
  vec2 texCoord;
} vertex_out;

void main()
{
  vertex_out.color = a_color;
  vertex_out.texCoord = (ubo.texture_matrix * a_texCoord).xy;
  float displace = (2.0 * texture(u_sampler_2D[DISPLACE], vertex_out.texCoord.st).x) - 1.0;
  vec4 displace_vert = a_vertex + vec4(a_normal * u_displace_factor * displace, 0.f);
  gl_Position = ubo.model_view_projection * displace_vert;
}
