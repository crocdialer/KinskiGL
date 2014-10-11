#version 330

uniform int u_numTextures; 
uniform sampler2D u_textureMap[16]; 

uniform struct
{
  vec4 diffuse; 
  vec4 ambient; 
  vec4 specular; 
  vec4 emission; 
  float shinyness; 
} u_material; 

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
  vec4 texColors = texture(u_textureMap[0], vertex_in.texCoord.xy); 
  vec3 N; 
  N = normalFromHeightMap(u_textureMap[1], vertex_in.texCoord.xy, 0.8); 
  vec3 L = normalize(-vertex_in.lightDir); 
  vec3 E = normalize(vertex_in.eyeVec); 
  vec3 R = reflect(-L, N); 
  float nDotL = max(0.0, dot(N, L));
  float specIntesity = pow( max(dot(R, E), 0.0), u_material.shinyness);
  vec4 spec = u_material.specular * specIntesity;
  spec.a = 0.0;
  fragData = texColors * (u_material.ambient + u_material.diffuse * vec4(vec3(nDotL), 1.0)) + spec; 
}
