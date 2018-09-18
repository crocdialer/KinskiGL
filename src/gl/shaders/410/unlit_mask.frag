#version 410 core
#extension GL_ARB_separate_shader_objects : enable

uniform sampler2D u_sampler_2D[2];

#define COLOR 0
#define MASK 1

struct Material
{
    vec4 diffuse;
    vec4 emission;
    vec4 point_vals;// (size, constant_att, linear_att, quad_att)
    float metalness;
    float roughness;
    float occlusion;
    int shadow_properties;
    int texture_properties;
};

layout(std140) uniform MaterialBlock
{
    Material u_material;
};

in VertexData
{
    vec4 color;
    vec2 texCoord;
} vertex_in;

out vec4 fragData;

void main()
{
    vec4 texColors = vertex_in.color;
    texColors *= texture(u_sampler_2D[COLOR], vertex_in.texCoord.st);
    float mask = texture(u_sampler_2D[MASK], vertex_in.texCoord.st).x;
    texColors.a *= mask;
    fragData = u_material.diffuse * texColors;
}
