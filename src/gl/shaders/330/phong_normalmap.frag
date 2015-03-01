#version 330

uniform int u_numTextures; 
uniform sampler2D u_sampler_2D[4]; 

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

in VertexData
{ 
  vec4 color; 
  vec4 texCoord; 
  vec3 normal; 
  vec3 eyeVec; 
  vec3 lightDir; 
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
  vec4 texColors = texture(u_sampler_2D[0], vertex_in.texCoord.xy); 
  vec3 N; 
  N = normalFromHeightMap(u_sampler_2D[1], vertex_in.texCoord.xy, 0.8); 
  vec3 L = normalize(-vertex_in.lightDir); 
  vec3 E = normalize(vertex_in.eyeVec); 
  vec3 R = reflect(-L, N); 
  float nDotL = max(0.0, dot(N, L));
  float specIntesity = pow( max(dot(R, E), 0.0), u_material.shinyness);
  vec4 spec = u_material.specular * specIntesity;
  spec.a = 0.0;
  fragData = texColors * (u_material.ambient + u_material.diffuse * vec4(vec3(nDotL), 1.0)) + spec; 
}
