uniform mat4 u_modelViewProjectionMatrix;
uniform float u_pointSize;
attribute vec4 a_vertex;
attribute vec4 a_color;
attribute float a_pointSize;
varying lowp vec4 v_color;

void main()
{
  gl_Position = u_modelViewProjectionMatrix * a_vertex;
  gl_PointSize = a_pointSize;
  v_color = a_color;
}
