#version 410 core
#extension GL_ARB_separate_shader_objects : enable

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

uniform vec2 u_texture_size = vec2(1.0);

layout(location = 0) in vec4 a_vertex;
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
  vertex_out.texCoord = (ubo.texture_matrix * a_texCoord).xy * u_texture_size;
  gl_Position = ubo.model_view_projection * a_vertex;
}
