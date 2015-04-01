#version 410

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

vec3 projected_coords(in vec4 the_lightspace_pos)
{
    vec3 proj_coords = the_lightspace_pos.xyz / the_lightspace_pos.w;
    proj_coords = (vec3(1) + proj_coords) * 0.5;
    return proj_coords;
}

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


//uniform Material u_material;
layout(std140) uniform MaterialBlock
{
  Material u_material;
};

layout(std140) uniform LightBlock
{
  int u_numLights;
  Lightsource u_lights[16];
};

uniform int u_numTextures;

uniform sampler2D u_sampler_2D[4];

#define EPSILON 0.00001
uniform sampler2D u_shadow_map[4];
uniform vec2 u_shadow_map_size = vec2(1024);

in VertexData
{
  vec4 color;
  vec4 texCoord;
  vec3 normal; 
  vec3 eyeVec; 
  vec4 lightspace_pos[4];
} vertex_in;

out vec4 fragData; 

float shadow_factor(int shadow_index)
{
  vec3 proj_coords = projected_coords(vertex_in.lightspace_pos[shadow_index]);
  float xOffset = 1.0/u_shadow_map_size.x;
  float yOffset = 1.0/u_shadow_map_size.y;
  float factor = 0.0;

  for(int y = -2 ; y <= 2 ; y++) 
  {
    for (int x = -2 ; x <= 2 ; x++) 
    {
      vec2 offset = vec2(x * xOffset, y * yOffset);
      float depth = texture(u_shadow_map[shadow_index], proj_coords.xy + offset).x;
      bool is_in_shadow = depth < (proj_coords.z - EPSILON);
      factor += is_in_shadow ? 0 : 1;
    }
  }
  return (0.5 + (factor / 50.0));
}

void main() 
{
  vec4 texColors = vec4(1); 
  
  for(int i = 0; i < u_numTextures; i++) 
    texColors *= texture(u_sampler_2D[i], vertex_in.texCoord.st); 
  
  vec3 normal = normalize(vertex_in.normal); 
  vec4 shade_color = vec4(0); 
  
  for(int i = 0; i < u_numLights; i++)
    shade_color += shade(u_lights[i], u_material, normal, vertex_in.eyeVec, texColors);
  
  
  shade_color.xyz *= shadow_factor(0);
  //shade_color.rgb = is_in_shadow ? vec3(1, 0 ,0) : shade_color.rgb ;

  fragData = shade_color; 
}
