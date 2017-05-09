#version 410

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
layout(location = 3) out vec4 out_specular;

void main()
{
  vec4 texColors = vertex_in.color;
  if(u_numTextures > 0){ texColors *= texture(u_sampler_2D[0], vertex_in.texCoord.st); }

  out_color = u_material.diffuse * texColors;
  out_normal = vec4(vertex_in.normal, 1);
  out_position = vec4(vertex_in.eyeVec, 1);
  out_specular = vec4(u_material.specular.rgb, u_material.shinyness);
}
