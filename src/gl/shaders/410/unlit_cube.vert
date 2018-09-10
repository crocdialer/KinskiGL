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

layout(location = 0) in vec4 a_vertex;

out VertexData
{
    vec3 eyeVec;
} vertex_out;

void main()
{
    vertex_out.eyeVec = a_vertex.xyz;
    gl_Position = ubo.model_view_projection * a_vertex;
}
