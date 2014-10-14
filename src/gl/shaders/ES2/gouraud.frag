uniform int u_numTextures;
uniform sampler2D u_sampler_2D[1]; 

varying vec4 v_color;
varying vec4 v_texCoord; 

void main() 
{
  vec4 texColors = vec4(1); 
  
  if(u_numTextures > 0)
  {
    texColors *= texture(u_sampler_2D[0], v_texCoord.st); 
  } 
  gl_FragColor = v_color * texColors; 
}
