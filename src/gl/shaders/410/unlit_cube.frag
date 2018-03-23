#version 410

uniform int u_numTextures;
uniform samplerCube u_sampler_cube[1];

struct Material
{
  vec4 diffuse;
  vec4 ambient;
  vec4 specular;
  vec4 emission;
  vec4 point_vals;// (size, constant_att, linear_att, quad_att)
  float shinyness;
};

layout(std140) uniform MaterialBlock
{
  Material u_material;
};

in VertexData
{
  vec4 color;
  vec2 texCoord;
  vec3 eyeVec;
} vertex_in;

out vec4 fragData;

void main()
{
    vec4 texColors = vertex_in.color;
    texColors.rgb = texture(u_sampler_cube[0], vertex_in.eyeVec).rgb;
    fragData = u_material.diffuse * texColors;
}
