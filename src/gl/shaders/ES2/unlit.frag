precision mediump float;
precision lowp int;
uniform int u_numTextures;
uniform sampler2D u_sampler_2D[1];

struct Material
{
  vec4 diffuse;
  vec4 ambient;
  vec4 specular;
  vec4 emission;
  vec4 point_vals;// (size, constant_att, linear_att, quad_att)
  float shinyness;
};
uniform Material u_material;

varying vec4 v_color;
varying vec4 v_texCoord;

void main(void)
{
  vec4 texColors = v_color;
  if(u_numTextures > 0)
    texColors *= texture2D(u_sampler_2D[0], v_texCoord.st);

  gl_FragColor = u_material.diffuse * texColors;
}
