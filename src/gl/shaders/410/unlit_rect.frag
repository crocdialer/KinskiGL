#version 410

uniform int u_numTextures;
uniform sampler2DRect u_sampler_2Drect[1];

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
  vec2 texCoord;
} vertex_in;

out vec4 fragData;

void main()
{
  vec4 texColors = vertex_in.color;
  texColors *= texture(u_sampler_2Drect[0], vertex_in.texCoord.st);
  fragData = u_material.diffuse * texColors;
}
