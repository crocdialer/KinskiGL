#version 410

uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;

// {TL, TR, BL, BR}
uniform vec2 u_control_points[36];
uniform vec2 u_num_subdivisions = vec2(1, 1);

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

  // find indices of relevant control_points
  vec2 pos = a_vertex.xy * u_num_subdivisions;

  // find TL corner
  int row_step = int(u_num_subdivisions.x + 1.0);
  int index_tl = int(pos.x) + row_step * int(u_num_subdivisions.y - floor(pos.y) - 1.0);
  int index_bl = index_tl + row_step;

  // interpolate top edge x coordinate
  vec2 x2 = mix(u_control_points[index_tl], u_control_points[index_tl + 1],
                fract(pos.x));

  // interpolate bottom edge x coordinate
  vec2 x1 = mix(u_control_points[index_bl], u_control_points[index_bl + 1],
                fract(pos.x));

  // interpolate y position
  vec2 p = mix(x1, x2, fract(pos.y));

  // interpolate bottom edge x coordinate
  // vec2 x1 = mix(u_control_points[2], u_control_points[3], a_vertex.x);
  //
  // // interpolate top edge x coordinate
  // vec2 x2 = mix(u_control_points[0], u_control_points[1], a_vertex.x);
  //
  // // interpolate y position
  // vec2 p = mix(x1, x2, a_vertex.y);

  vertex_out.color = a_color;
  vertex_out.texCoord = (u_textureMatrix * a_texCoord).xy;
  gl_Position = u_modelViewProjectionMatrix * vec4(p, 0, 1);
}
