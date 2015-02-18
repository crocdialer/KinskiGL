#version 330

struct Material
{
  vec4 diffuse; 
  vec4 ambient; 
  vec4 specular; 
  vec4 emission; 
  float shinyness;
  float point_size; 
  struct
  {
    float constant; 
    float linear; 
    float quadratic; 
  } point_attenuation;
};

uniform int u_numLights; 

struct Lightsource
{
  int type; 
  vec3 position; 
  vec4 diffuse; 
  vec4 ambient; 
  vec4 specular; 
  float constantAttenuation; 
  float linearAttenuation; 
  float quadraticAttenuation; 
  vec3 spotDirection; 
  float spotCosCutoff; 
  float spotExponent; 
};

vec4 shade(in Lightsource light, in Material mat, in vec3 normal, in vec3 eyeVec, in vec4 base_color)
{
  vec3 lightDir = light.type > 0 ? (light.position - eyeVec) : -light.position; 
  vec3 L = normalize(lightDir); 
  vec3 E = normalize(-eyeVec); 
  vec3 R = reflect(-L, normal); 
  vec4 ambient = mat.ambient * light.ambient; 
  float att = 1.0; 
  float nDotL = dot(normal, L); 
  if (light.type > 0)
  {
    float dist = length(lightDir); 
    att = 1.0 / (light.constantAttenuation + light.linearAttenuation * dist + light.quadraticAttenuation * dist * dist); 
    
    if(light.type > 1) 
    {
      float spotEffect = dot(normalize(light.spotDirection), -L); 
      
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
  vec4 diffuse = att * mat.diffuse * light.diffuse * vec4(vec3(nDotL), 1.0); 
  vec4 spec = att * mat.specular * light.specular * specIntesity; 
  spec.a = 0.0; 
  return base_color * (ambient + diffuse) + spec; 
}

uniform float u_pointRadius; 
uniform vec3 u_lightDir; 
uniform int u_numTextures; 
uniform sampler2D u_sampler_2D[8]; 
uniform Lightsource u_lights[16]; 
uniform Material u_material; 

in vec4 v_color; 
in vec3 v_eyeVec; 

out vec4 fragData; 

void main() 
{
  vec4 texColors = v_color; 
  
  for(int i = 0; i < u_numTextures; i++) 
  { 
    texColors *= texture(u_sampler_2D[i], gl_PointCoord); 
  }
  vec3 N; 
  N.xy = gl_PointCoord * vec2(2.0, -2.0) + vec2(-1.0, 1.0); 
  float mag = dot(N.xy, N.xy); 
  
  if (mag > 1.0) discard; 
  
  N.z = sqrt(1.0 - mag); 
  vec3 spherePosEye = v_eyeVec + N * u_pointRadius; 
  vec3 L = normalize(-u_lightDir); 
  vec3 E = normalize(v_eyeVec); 
  float nDotL = max(0.0, dot(N, L)); 
  vec3 v = normalize(-spherePosEye); 
  vec3 h = normalize(-u_lightDir + v); 
  float specIntesity = pow( max(dot(N, h), 0.0), u_material.shinyness); 
  vec4 spec = u_material.specular * specIntesity; 
  spec.a = 0.0; 
  fragData = texColors * (u_material.ambient + u_material.diffuse * vec4(vec3(nDotL), 1.0)) + spec; 
}
