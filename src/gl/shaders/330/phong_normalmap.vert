#version 330

uniform mat4 u_modelViewMatrix; 
uniform mat4 u_modelViewProjectionMatrix; 
uniform mat3 u_normalMatrix; 
uniform mat4 u_textureMatrix; 
uniform vec3 u_lightDir; 

layout(location = 0) in vec4 a_vertex; 
layout(location = 1) in vec3 a_normal; 
layout(location = 2) in vec4 a_texCoord; 
layout(location = 5) in vec3 a_tangent; 

out VertexData
{
  vec4 color; 
  vec4 texCoord; 
  vec3 normal; 
  vec3 eyeVec; 
  vec3 lightDir; 
} vertex_out; 

void main()
{
  vertex_out.normal = normalize(u_normalMatrix * a_normal); 
  vec3 t = normalize (u_normalMatrix * a_tangent); 
  vec3 b = cross(vertex_out.normal, t); 
  mat3 tbnMatrix = mat3(t,b, vertex_out.normal); 
  vertex_out.eyeVec = tbnMatrix * normalize(- (u_modelViewMatrix * a_vertex).xyz); 
  vertex_out.lightDir = tbnMatrix * u_lightDir; 
  vertex_out.texCoord = u_textureMatrix * a_texCoord; 
  gl_Position = u_modelViewProjectionMatrix * a_vertex; 
}
