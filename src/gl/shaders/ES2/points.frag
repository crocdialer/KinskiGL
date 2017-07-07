precision mediump float;
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

varying lowp vec4 v_color;

void main()
{
  vec4 texColors = v_color;
  // if(u_numTextures > 0)
  // {
  //   texColors *= texture2D(u_textureMap[i], gl_PointCoord);
  // }
  gl_FragColor = u_material.diffuse * texColors;
}
