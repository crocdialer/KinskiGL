uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;
uniform mat4 u_bones[64];

attribute vec4 a_vertex;
attribute vec4 a_texCoord;
attribute vec4 a_color;
attribute vec4 a_boneIds; 
attribute vec4 a_boneWeights;

varying lowp vec4 v_color;
varying lowp vec4 v_texCoord;

void main(void)
{
  v_color = a_color;
  v_texCoord =  u_textureMatrix * a_texCoord;
  
  vec4 newVertex = vec4(0); 
  
  for (int i = 0; i < 4; i++)
  {
    newVertex += u_bones[int(a_boneIds[i])] * a_vertex * a_boneWeights[i]; 
  }
  gl_Position = u_modelViewProjectionMatrix * vec4(newVertex.xyz, 1.0);
}

