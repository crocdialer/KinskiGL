#version 410

uniform int u_numTextures;
uniform sampler2D u_sampler_2D[1]; 

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

layout(std140) uniform MaterialBlock
{
  Material u_material;
};

in vec4 v_color; 

out vec4 fragData; 

void main() 
{
  vec4 texColors = v_color; 
  
  if(u_numTextures > 0)
  {
    texColors *= texture(u_sampler_2D[0], gl_PointCoord.xy); 
  } 
  fragData = u_material.diffuse * texColors; 
}
