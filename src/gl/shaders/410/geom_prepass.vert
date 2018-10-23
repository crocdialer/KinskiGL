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

in vec4 a_vertex;
in vec3 a_normal;
in vec4 a_color;
in vec2 a_texCoord;
in float a_pointSize;

out VertexData
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec2 texCoord;
    float pointSize;
} vertex_out;

void main()
{
    vertex_out.position = a_vertex.xyz;
    vertex_out.normal = a_normal;
    vertex_out.pointSize = a_pointSize;
    vertex_out.color = a_color;
    vertex_out.texCoord =  (ubo.texture_matrix * vec4(a_texCoord, 0, 1)).xy;
}
