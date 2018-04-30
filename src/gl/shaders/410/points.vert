#version 410

uniform mat4 u_modelViewMatrix;
uniform mat4 u_modelViewProjectionMatrix;

struct Material
{
    vec4 diffuse;
    vec4 ambient;
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

layout(location = 0) in vec4 a_vertex;
layout(location = 3) in vec4 a_color;
layout(location = 4) in float a_pointSize;

out VertexData
{
  vec4 color;
  vec3 eyeVec;
  float point_size;
} vertex_out;

void main()
{
  vertex_out.color = a_color;
  vertex_out.eyeVec = -(u_modelViewMatrix * a_vertex).xyz;
  float d = length(vertex_out.eyeVec);
  float attenuation = 1.0 / (u_material.point_vals[1] + u_material.point_vals[2] * d + u_material.point_vals[3] * (d * d));
  gl_PointSize = vertex_out.point_size = max(a_pointSize, u_material.point_vals[0]) * attenuation;
  gl_Position = u_modelViewProjectionMatrix * a_vertex;
}
