#version 410 core
#extension GL_ARB_separate_shader_objects : enable

uniform int u_numTextures;
uniform sampler2D u_sampler_2D[1];

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
  vec3 eyeVec;
  float point_size;
} vertex_in;

out vec4 fragData;

void main()
{
  vec4 texColors = vertex_in.color;

  if(u_numTextures > 0)
  {
    texColors *= texture(u_sampler_2D[0], gl_PointCoord.xy);
  }
  fragData = u_material.diffuse * texColors;
}
