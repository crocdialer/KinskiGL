uniform int u_numTextures;
uniform sampler2D u_sampler_2D[1];

varying vec4 v_color;
varying vec4 v_texCoord;

void main()
{
  vec4 texColors = texture2D(u_sampler_2D[0], v_texCoord.st); 
  gl_FragColor = v_color * texColors;
}
