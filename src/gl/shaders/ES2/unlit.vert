uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;
attribute vec4 a_vertex;
attribute vec4 a_texCoord;
attribute vec4 a_color;
varying lowp vec4 v_color;
varying lowp vec4 v_texCoord;

void main(void)
{
  v_color = a_color;
  v_texCoord =  u_textureMatrix * a_texCoord;
  gl_Position = u_modelViewProjectionMatrix * a_vertex;
}
