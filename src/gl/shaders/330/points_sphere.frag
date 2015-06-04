#version 330

struct Material
{
  vec4 diffuse; 
  vec4 ambient; 
  vec4 specular; 
  vec4 emission; 
  vec4 point_vals;// (size, constant_att, linear_att, quad_att) 
  float shinyness;
};

struct Lightsource
{
  vec3 position; 
  int type; 
  vec4 diffuse; 
  vec4 ambient; 
  vec4 specular; 
  vec3 spotDirection; 
  float spotCosCutoff; 
  float spotExponent; 
  float constantAttenuation; 
  float linearAttenuation; 
  float quadraticAttenuation; 
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

layout(std140) uniform MaterialBlock
{
  Material u_material;
};

layout(std140) uniform LightBlock
{
  int u_numLights;
  Lightsource u_lights[16];
}; 

in VertexData
{
  vec4 color; 
  vec3 eyeVec;
  float point_size;
} vertex_in; 

out vec4 fragData; 

void main() 
{
  vec4 texColors = vertex_in.color; 
  
  //for(int i = 0; i < u_numTextures; i++)
  if(u_numTextures > 0)
  { 
    texColors *= texture(u_sampler_2D[0], gl_PointCoord); 
  }
  vec3 normal; 
  normal.xy = gl_PointCoord * vec2(2.0, -2.0) + vec2(-1.0, 1.0); 
  float mag = dot(normal.xy, normal.xy); 
  
  if (mag > 1.0) discard; 
  
  normal.z = sqrt(1.0 - mag); 
  vec3 spherePosEye = -(vertex_in.eyeVec + normal * vertex_in.point_size / 2.0); 

  vec4 shade_color = vec4(0); 
  
  if(u_numLights > 0) 
    shade_color += shade(u_lights[0], u_material, normal, spherePosEye, texColors); 
  
  if(u_numLights > 1)
    shade_color += shade(u_lights[1], u_material, normal, spherePosEye, texColors);

  if(u_numLights > 2) 
    shade_color += shade(u_lights[2], u_material, normal, spherePosEye, texColors); 
  
  if(u_numLights > 3)
    shade_color += shade(u_lights[3], u_material, normal, spherePosEye, texColors);

  fragData = shade_color;//texColors * (u_material.diffuse * vec4(vec3(nDotL), 1.0)) + spec; 
}
