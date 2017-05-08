#version 410

uniform int u_numTextures;
uniform sampler2D u_sampler_2D[1];

// {TL, TR, BL, BR}
uniform vec2[4] u_control_points;

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
} vertex_in;

out vec4 fragData;

void main()
{
  vec4 texColors = vertex_in.color;

  // interpolate bottom edge x coordinate
  vec2 x1 = mix(u_control_points[2], u_control_points[3], vertex_in.texCoord.x);

  // interpolate top edge x coordinate
  vec2 x2 = mix(u_control_points[0], u_control_points[1], vertex_in.texCoord.x);

  // interpolate y position
  vec2 p = mix(x1, x2, vertex_in.texCoord.y);

  if(p.x < 0.0 || p.y < 0.0 || p.x > 1.0 || p.y > 1.0) { discard; }

  texColors *= texture(u_sampler_2D[0], p);
  fragData = u_material.diffuse * texColors;
}
