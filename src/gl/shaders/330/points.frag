#version 330

uniform int u_numTextures;
uniform sampler2D u_sampler_2D[1]; 

uniform struct
{
  vec4 diffuse; 
  vec4 ambient; 
  vec4 specular; 
  vec4 emission; 
} u_material; 

in vec4 v_color; 

out vec4 fragData; 

void main() 
{
  vec4 texColors = v_color; 
  
  if(u_numTextures > 0)
  {
    texColors *= texture(u_sampler_2D[i], gl_PointCoord.xy); 
  } 
  fragData = u_material.diffuse * texColors; 
}
