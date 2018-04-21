#version 410

uniform int u_numTextures;
uniform sampler2D u_sampler_2D[1];

#define ROUGH_METAL_MAP 0

struct Material
{
    vec4 diffuse;
    vec4 ambient;
    vec4 emission;
    vec4 point_vals;// (size, constant_att, linear_att, quad_att)
    float metalness;
    float roughness;
    int shadow_properties;
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
layout(location = 4) out vec4 out_material_props;

void main()
{
    out_color = u_material.diffuse * vertex_in.color;
    out_normal = vec4(vertex_in.normal, 1);
    out_position = vec4(vertex_in.eyeVec, 1);
    out_emission = u_material.emission * vertex_in.color;
    vec4 rough_metal = texture(u_sampler_2D[ROUGH_METAL_MAP], vertex_in.texCoord.xy);
    out_material_props = vec4(rough_metal.b, rough_metal.g, u_material.shadow_properties & 2, 1);
}
