precision mediump float;
uniform mat4 u_modelViewProjectionMatrix;
uniform float u_pointSize;

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
uniform Material u_material;

attribute vec4 a_vertex;
attribute vec4 a_color;
attribute float a_pointSize;

varying lowp vec4 v_color;

void main()
{
  gl_Position = u_modelViewProjectionMatrix * a_vertex;
  gl_PointSize = a_pointSize * u_material.point_vals.x;
  v_color = a_color;
}
