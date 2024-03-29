#version 410 core
#extension GL_ARB_separate_shader_objects : enable

uniform int u_numTextures;
uniform sampler2D u_sampler_2D[1];

#define AO_ROUGH_METAL 0

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
    vec4 texCoord;
    vec3 normal;
    vec3 eyeVec;
} vertex_in;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_position;
layout(location = 3) out vec4 out_emission;
layout(location = 4) out vec4 out_ao_rough_metal;

void main()
{
    out_color = u_material.diffuse * vertex_in.color;
    out_normal = vec4(vertex_in.normal, 1);
    out_position = vec4(vertex_in.eyeVec, 1);
    out_emission = u_material.emission * vertex_in.color;
    out_ao_rough_metal = texture(u_sampler_2D[AO_ROUGH_METAL], vertex_in.texCoord.xy);
}
