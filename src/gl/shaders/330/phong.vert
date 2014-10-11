#version 330

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

uniform mat4 u_modelViewMatrix; 
uniform mat4 u_modelViewProjectionMatrix; 
uniform mat3 u_normalMatrix; 
uniform mat4 u_textureMatrix; 

in vec4 a_vertex; 
in vec3 a_normal; 
in vec4 a_texCoord; 

out VertexData
{
  vec4 color; 
  vec4 texCoord; 
  vec3 normal; 
  vec3 eyeVec; 
} vertex_out; 

void main()
{
  vertex_out.normal = normalize(u_normalMatrix * a_normal); 
  vertex_out.texCoord = u_textureMatrix * a_texCoord; 
  vertex_out.eyeVec = (u_modelViewMatrix * a_vertex).xyz; 
  gl_Position = u_modelViewProjectionMatrix * a_vertex; 
}
