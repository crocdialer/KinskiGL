uniform mat4 u_modelViewMatrix;
uniform mat4 u_modelViewProjectionMatrix;
uniform mat3 u_normalMatrix;
uniform mat4 u_textureMatrix;
uniform mat4 u_bones[18];
attribute vec4 a_vertex;
attribute vec4 a_texCoord;
attribute vec3 a_normal;
attribute vec4 a_boneIds;
attribute vec4 a_boneWeights;
varying vec4 v_texCoord;
varying vec3 v_normal;
varying vec3 v_eyeVec;

void main()
{
  vec4 newVertex = vec4(0.0);
  vec4 newNormal = vec4(0.0);
  
  for (int i = 0; i < 4; i++)
  {
    newVertex += u_bones[int(floor(a_boneIds[i]))] * a_vertex * a_boneWeights[i];
    newNormal += u_bones[int(floor(a_boneIds[i]))] * vec4(a_normal, 0.0) * a_boneWeights[i];
  }

  v_normal = normalize(u_normalMatrix * newNormal.xyz);
  v_texCoord =  u_textureMatrix * a_texCoord;
  v_eyeVec = - (u_modelViewMatrix * newVertex).xyz;
  gl_Position = u_modelViewProjectionMatrix * vec4(newVertex.xyz, 1.0);
}
