#version 410

uniform mat4 u_modelViewMatrix; 
uniform mat4 u_modelViewProjectionMatrix; 

struct Material
{
  vec4 diffuse; 
  vec4 ambient; 
  vec4 specular; 
  vec4 emission; 
  float shinyness;
  float point_size; 
  struct
  {
    float constant; 
    float linear; 
    float quadratic; 
  } point_attenuation;
}; 

layout(location = 0) in vec4 a_vertex; 
//layout(location = 1) in vec3 a_normal; 
//layout(location = 2) in vec4 a_texCoord;
layout(location = 3) in vec4 a_color; 
layout(location = 4) in float a_pointSize; 

out vec4 v_color; 
out vec3 v_eyeVec; 

void main()
{
  v_color = a_color; 
  v_eyeVec = -(u_modelViewMatrix * a_vertex).xyz; 
  float d = length(v_eyeVec); 
  float attenuation = 1.0 / (u_point_attenuation.constant + u_point_attenuation.linear * d + u_point_attenuation.quadratic * (d * d)); 
  gl_PointSize = max(a_pointSize, u_pointSize) * attenuation; 
  gl_Position = u_modelViewProjectionMatrix * a_vertex; 
}
