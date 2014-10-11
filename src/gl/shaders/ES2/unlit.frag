precision mediump float;
precision lowp int;
uniform int u_numTextures;
uniform sampler2D u_textureMap[2];

uniform struct
{
  vec4 diffuse;
  vec4 ambient;
  vec4 specular;
  vec4 emission;
  float shinyness;
} u_material;

varying vec4 v_color;
varying vec4 v_texCoord;

void main()
{
  vec4 texColors = v_color;
  if(u_numTextures > 0)
    texColors *= texture2D(u_textureMap[0], v_texCoord.st);

  if(u_numTextures > 1)
    texColors *= texture2D(u_textureMap[1], v_texCoord.st);

  gl_FragColor = u_material.diffuse * texColors;
}
