#version 410

uniform int u_numTextures; 
uniform sampler2D u_sampler_2D[4]; 

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

layout(std140) uniform MaterialBlock
{
  Material u_material;
};

layout(std140) uniform LightBlock
{
  int u_numLights;
  Lightsource u_lights[16];
};

vec4 shade(in Lightsource light, in Material mat, in vec3 normal, in vec3 eyeVec, in vec3 lightDir,
    in vec4 base_color)
{
//  vec3 lightDir = light.type > 0 ? (light.position - eyeVec) : -light.position; 
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

in VertexData
{ 
  vec4 color; 
  vec4 texCoord; 
  vec3 eyeVec; 
  vec3 lightDir[16]; 
} vertex_in; 

out vec4 fragData; 

vec3 normalFromHeightMap(sampler2D theMap, vec2 theCoords, float theStrength) 
{
  float center = texture(theMap, theCoords).r;
  float U = texture(theMap, theCoords + vec2( 0.005, 0)).r;
  float V = texture(theMap, theCoords + vec2(0, 0.005)).r;
  float dHdU = U - center; 
  float dHdV = V - center; 
  vec3 normal = vec3( -dHdU, dHdV, 0.05 / theStrength); 
  return normalize(normal);
}

void main()
{
  vec4 texColors = vec4(1);//vertex_in.color;//texture(u_sampler_2D[0], vertex_in.texCoord.xy); 
  vec3 normal; 
  //normal = normalFromHeightMap(u_sampler_2D[1], vertex_in.texCoord.xy, 0.8); 
  normal = normalize(2 * (texture(u_sampler_2D[1], vertex_in.texCoord.xy).xyz - vec3(0.5))); 
  vec4 shade_color = vec4(0); 
  
  for(int i = 0; i < u_numLights; i++)
  {
    shade_color += shade(u_lights[i], u_material, normal, vertex_in.eyeVec,
        vertex_in.lightDir[i], texColors);
  }
  fragData = shade_color; 
}
