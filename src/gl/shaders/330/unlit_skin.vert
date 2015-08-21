#version 330

uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;
uniform mat4 u_bones[110];

layout(location = 0) in vec4 a_vertex; 
layout(location = 2) in vec4 a_texCoord;
layout(location = 3) in vec4 a_color; 
layout(location = 6) in ivec4 a_boneIds; 
layout(location = 7) in vec4 a_boneWeights;

out VertexData
{ 
  vec4 color;
  vec2 texCoord;
} vertex_out;

void main() 
{
  vertex_out.color = a_color;
  vertex_out.texCoord = (u_textureMatrix * a_texCoord).xy;
  vec4 newVertex = vec4(0); 
  
  for (int i = 0; i < 4; i++)
  {
    newVertex += u_bones[a_boneIds[i]] * a_vertex * a_boneWeights[i]; 
  }
  gl_Position = u_modelViewProjectionMatrix * vec4(newVertex.xyz, 1.0); 
}
