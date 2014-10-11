#version 330

uniform mat4 u_modelViewMatrix; 
uniform mat4 u_modelViewProjectionMatrix; 
uniform float u_pointSize; 

uniform struct
{
  float constant; 
  float linear; 
  float quadratic; 
} u_point_attenuation; 

in vec4 a_vertex; 
in float a_pointSize; 
in vec4 a_color; 

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
