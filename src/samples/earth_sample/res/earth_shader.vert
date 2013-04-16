#version 150 core
uniform mat4 u_modelViewMatrix;
uniform mat4 u_modelViewProjectionMatrix;
uniform mat3 u_normalMatrix;
uniform mat4 u_textureMatrix;
uniform vec3 u_lightDir;
in vec4 a_vertex;
in vec4 a_texCoord;
in vec3 a_normal;
in vec3 a_tangent;
out vec4 v_texCoord;
out vec3 v_normal;
out vec3 v_eyeVec;
out vec3 v_lightDir;
void main()
{
   v_normal = normalize(u_normalMatrix * a_normal);
   vec3 t = normalize (u_normalMatrix * a_tangent);
   vec3 b = cross(v_normal, t);
   mat3 tbnMatrix = mat3(t,b, v_normal);
   v_eyeVec = tbnMatrix * normalize(- (u_modelViewMatrix * a_vertex).xyz);
   v_lightDir = tbnMatrix * normalize(u_lightDir);
   v_texCoord =  u_textureMatrix * a_texCoord;
    gl_Position = u_modelViewProjectionMatrix * a_vertex;
}