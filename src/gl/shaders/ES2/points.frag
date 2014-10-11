uniform int u_numTextures;
uniform sampler2D u_sampler_2D[1];

uniform struct
{
  vec4 diffuse;
  vec4 ambient;
  vec4 specular;
  vec4 emission;
  float shinyness;
} u_material;

varying vec4 v_color;

void main()
{
  vec4 texColors = v_color;
  if(u_numTextures > 0)
  {
    texColors *= texture2D(u_textureMap[i], gl_PointCoord);
  }
  gl_FragColor = u_material.diffuse * texColors;
}
