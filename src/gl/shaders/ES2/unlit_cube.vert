uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;

attribute vec4 a_vertex;

varying lowp vec3 v_eyeVec;

void main()
{
    v_eyeVec = a_vertex.xyz;
    gl_Position = u_modelViewProjectionMatrix * a_vertex;
}
