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

uniform int u_numLights;

struct Lightsource
{
    vec3 position;
    int type;
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    vec3 direction;
    float intensity;
    float spotCosCutoff;
    float spotExponent;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
};

vec4 shade(in Lightsource light, in Material mat, in vec3 normal, in vec3 eyeVec, in vec4 base_color,
           float shade_factor)
{
  vec3 lightDir = light.type > 0 ? (light.position - eyeVec) : -light.direction;
  vec3 L = normalize(lightDir);
  vec3 E = normalize(-eyeVec);
  vec3 R = reflect(-L, normal);
  vec4 ambient = mat.ambient * light.ambient;
  float att = 1.0;
  float nDotL = dot(normal, L);

  if (light.type > 0)
  {
    float dist = length(lightDir);
    att = min(1.f, light.intensity / (light.constantAttenuation +
                   light.linearAttenuation * dist +
                   light.quadraticAttenuation * dist * dist));

    if(light.type > 1)
    {
      float spotEffect = dot(normalize(light.direction), -L);

      if (spotEffect < light.spotCosCutoff)
      {
        att = 0.0;
        base_color * ambient;
      }
      spotEffect = pow(spotEffect, light.spotExponent);
      att *= spotEffect;
    }
  }
  nDotL = max(0.0, nDotL);
  float specIntesity = clamp(pow( max(dot(R, E), 0.0), mat.shinyness), 0.0, 1.0);
  vec4 diffuse = mat.diffuse * light.diffuse * vec4(att * shade_factor * vec3(nDotL), 1.0);
  vec4 spec = shade_factor * att * mat.specular * light.specular * specIntesity;
  spec.a = 0.0;
  return base_color * (ambient + diffuse) + spec;
}

uniform mat4 u_modelViewMatrix; 
uniform mat4 u_modelViewProjectionMatrix; 
uniform mat3 u_normalMatrix; 
uniform mat4 u_textureMatrix; 
uniform Material u_material; 
uniform Lightsource u_lights[2]; 

attribute vec4 a_vertex; 
attribute vec3 a_normal; 
attribute vec4 a_texCoord; 

varying vec4 v_color;
varying vec4 v_texCoord; 

void main() 
{
  v_texCoord = u_textureMatrix * a_texCoord;
  vec3 normal = normalize(u_normalMatrix * a_normal); 
  vec3 eyeVec = (u_modelViewMatrix * a_vertex).xyz;
  vec4 shade_color = vec4(0);

  if(u_numLights > 0)
    shade_color += shade(u_lights[0], u_material, normal, eyeVec, vec4(1), 1.0);
  
  if(u_numLights > 1)
    shade_color += shade(u_lights[1], u_material, normal, eyeVec, vec4(1), 1.0);
  
  v_color = shade_color; 
  gl_Position = u_modelViewProjectionMatrix * a_vertex; 
}
