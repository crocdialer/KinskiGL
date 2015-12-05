#version 330

uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;

// {TL, TR, BL, BR}
uniform vec2[4] u_control_points;

layout(location = 0) in vec4 a_vertex; 
layout(location = 2) in vec4 a_texCoord;
layout(location = 3) in vec4 a_color; 

out VertexData
{ 
  vec4 color;
  vec2 texCoord;
} vertex_out;

void main() 
{
  // vertices are on a normalized quad
  
  // interpolate bottom edge x coordinate
  vec2 x1 = mix(u_control_points[2], u_control_points[3], a_vertex.x);

  // interpolate top edge x coordinate
  vec2 x2 = mix(u_control_points[0], u_control_points[1], a_vertex.x);

  // interpolate y position
  vec2 p = mix(x1, x2, a_vertex.y);

  vertex_out.color = a_color;
  vertex_out.texCoord = (u_textureMatrix * a_texCoord).xy;
  gl_Position = u_modelViewProjectionMatrix * vec4(p, 0, 1); 
}
