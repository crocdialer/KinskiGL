#version 410

uniform int u_numTextures;
uniform float u_gamma = 1.0;

uniform sampler2D u_sampler_2D[2];
#define COLOR 0

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
  if(u_numTextures > 0){ texColors *= texture(u_sampler_2D[COLOR], vertex_in.texCoord.st); }
  fragData = u_material.diffuse * texColors;
  if(u_gamma != 1.0){ fragData.rgb = pow(fragData.rgb, vec3(1.0 / u_gamma)); }
}
