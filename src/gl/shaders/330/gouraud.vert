#version 410

struct Material
{ 
   vec4 diffuse;
   vec4 ambient; 
   vec4 specular; 
   vec4 emission;
   float shinyness;
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
  vec4 spec = att * mat.specular * light.specular * specIntesity; spec.a = 0.0; 
  return base_color * (ambient + diffuse) + spec; 
}

uniform mat4 u_modelViewMatrix; 
uniform mat4 u_modelViewProjectionMatrix; 
uniform mat3 u_normalMatrix; 
uniform mat4 u_textureMatrix; 
uniform Material u_material; 
uniform Lightsource u_lights[16]; 

layout(location = 0) in vec4 a_vertex; 
layout(location = 1) in vec3 a_normal; 
layout(location = 2) in vec4 a_texCoord; 

out VertexData 
{
  vec4 color;
  vec4 texCoord; 
} vertex_out;

void main() 
{
  vertex_out.texCoord = u_textureMatrix * a_texCoord;
  vec3 normal = normalize(u_normalMatrix * a_normal); 
  vec3 eyeVec = (u_modelViewMatrix * a_vertex).xyz;
  vec4 shade_color = vec4(0);

  if(u_numLights > 0)
    shade_color += shade(u_lights[0], u_material, normal, eyeVec, vec4(1)); 
  
  if(u_numLights > 1)
    shade_color += shade(u_lights[1], u_material, normal, eyeVec, vec4(1));
  
  vertex_out.color = shade_color; 
  gl_Position = u_modelViewProjectionMatrix * a_vertex; 
}
