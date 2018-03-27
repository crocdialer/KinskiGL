precision mediump float;
precision lowp int;
uniform int u_numTextures;
uniform sampler2D u_sampler_2D[2];
uniform vec3 u_lightDir;

struct Material
{
    vec4 diffuse;
    vec4 ambient;
    vec4 emission;
    vec4 point_vals;// (size, constant_att, linear_att, quad_att)
    float metalness;
    float roughness;
    int shadow_properties;
}; 
uniform Material u_material;

varying vec3 v_normal;
varying vec4 v_texCoord;
varying vec3 v_eyeVec;

void main()
{
  vec4 texColors = vec4(1);
  if(u_numTextures > 0) texColors *= texture2D(u_sampler_2D[0], v_texCoord.st);
  if(u_numTextures > 1) texColors *= texture2D(u_sampler_2D[1], v_texCoord.st);

  vec3 N = normalize(v_normal);
  vec3 L = normalize(-u_lightDir);
  vec3 E = normalize(v_eyeVec);
  vec3 R = reflect(-L, N);

  float nDotL = max(0.0, dot(N, L));
  float specIntesity = pow( max(dot(R, E), 0.0), u_material.shinyness);
  vec4 spec = u_material.specular * specIntesity; spec.a = 0.0;
  gl_FragColor = texColors * (u_material.ambient + u_material.diffuse * vec4(vec3(nDotL), 1.0)) + spec;
}
