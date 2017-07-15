#version 310 es
precision mediump float;

uniform int u_numTextures;
uniform sampler2D u_sampler_2D[1];

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

in vec4 v_color;
in vec2 v_texCoord;

out vec4 fragData;

void main()
{
  vec4 texColors = v_color;
  if(u_numTextures > 0)
    texColors *= texture(u_sampler_2D[0], v_texCoord.st);
  fragData = u_material.diffuse * texColors;
}
