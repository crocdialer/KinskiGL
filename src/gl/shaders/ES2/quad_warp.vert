#version 330

uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;

// {TL, BL, BR, TR}
// {TL, TR, BL, BR}
uniform vec2[4] u_control_points;

attribute vec4 a_vertex;
attribute vec4 a_texCoord;
attribute vec4 a_color;
varying lowp vec4 v_color;
varying lowp vec4 v_texCoord;

void main() 
{
  // interpolate bottom edge x coordinate
  vec2 x1 = mix(u_control_points[2], u_control_points[3], a_vertex.x);

  // interpolate top edge x coordinate
  vec2 x2 = mix(u_control_points[0], u_control_points[1], a_vertex.x);

  // interpolate y position
  vec2 p = mix(x1, x2, a_vertex.y);

  v_color = a_color;
  v_texCoord =  u_textureMatrix * a_texCoord;
  gl_Position = u_modelViewProjectionMatrix * vec4(p, 0, 1); 
}
