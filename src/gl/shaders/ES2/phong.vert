uniform mat4 u_modelViewMatrix;
uniform mat4 u_modelViewProjectionMatrix;
uniform mat3 u_normalMatrix;
uniform mat4 u_textureMatrix;

attribute vec4 a_vertex;
attribute vec4 a_texCoord;
attribute vec3 a_normal;

varying lowp vec4 v_texCoord;
varying mediump vec3 v_normal;
varying mediump vec3 v_eyeVec;

void main()
{
  v_normal = normalize(u_normalMatrix * a_normal);
  v_texCoord =  u_textureMatrix * a_texCoord;
  v_eyeVec = - (u_modelViewMatrix * a_vertex).xyz;
  gl_Position = u_modelViewProjectionMatrix * a_vertex;
}
