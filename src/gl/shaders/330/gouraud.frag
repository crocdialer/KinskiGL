#version 330

uniform int u_numTextures;
uniform sampler2D u_sampler_2D[1]; 

in VertexData 
{
  vec4 color;
  vec4 texCoord; 
} vertex_in; 

out vec4 fragData; 

void main() 
{
  vec4 texColors = vec4(1); 
  
  if(u_numTextures > 0)
  {
    texColors *= texture(u_sampler_2D[0], vertex_in.texCoord.st); 
  } 
  fragData = vertex_in.color * texColors; 
}
