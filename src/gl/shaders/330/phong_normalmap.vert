#version 330

uniform mat4 u_modelViewMatrix; 
uniform mat4 u_modelViewProjectionMatrix; 
uniform mat3 u_normalMatrix; 
uniform mat4 u_textureMatrix; 

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

layout(std140) uniform LightBlock
{
  int u_numLights;
  Lightsource u_lights[16];
};

layout(location = 0) in vec4 a_vertex; 
layout(location = 1) in vec3 a_normal; 
layout(location = 2) in vec4 a_texCoord; 
layout(location = 5) in vec3 a_tangent; 

out VertexData
{
  vec4 color; 
  vec4 texCoord; 
  vec3 eyeVec; 
  vec3 lightDir[16]; 
} vertex_out; 

void main()
{
  vec3 n = normalize(u_normalMatrix * a_normal); 
  vec3 t = normalize (u_normalMatrix * a_tangent); 
  vec3 b = cross(n, t); 
  mat3 tbnMatrix = mat3(t, b, n);
  vec3 eye = (u_modelViewMatrix * a_vertex).xyz; 
  vertex_out.texCoord = u_textureMatrix * a_texCoord; 
  vertex_out.eyeVec = tbnMatrix * eye; 

  for(int i = 0; i < u_numLights; i++)
  {
    vertex_out.lightDir[i] = tbnMatrix * vec3(1, 0, 0);
  }

  gl_Position = u_modelViewProjectionMatrix * a_vertex; 
}
