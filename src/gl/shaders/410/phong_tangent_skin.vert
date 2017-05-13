#version 410

uniform mat4 u_modelViewMatrix;
uniform mat4 u_modelViewProjectionMatrix;
uniform mat3 u_normalMatrix;
uniform mat4 u_textureMatrix;
uniform mat4 u_bones[110];

layout(location = 0) in vec4 a_vertex;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_texCoord;
layout(location = 3) in vec4 a_color;
layout(location = 5) in vec3 a_tangent;
layout(location = 6) in ivec4 a_boneIds;
layout(location = 7) in vec4 a_boneWeights;

out VertexData
{
  vec4 color;
  vec4 texCoord;
  vec3 normal;
  vec3 eyeVec;
  vec3 tangent;
} vertex_out;

void main()
{
    vec4 skin_vertex = vec4(0);
    vec4 skin_normal = vec4(0);
    vec4 skin_tangent = vec4(0);

    for (int i = 0; i < 4; i++)
    {
        skin_vertex += u_bones[a_boneIds[i]] * a_vertex * a_boneWeights[i];
        skin_normal += u_bones[a_boneIds[i]] * vec4(a_normal, 0.0) * a_boneWeights[i];
        skin_tangent += u_bones[a_boneIds[i]] * vec4(a_tangent, 0.0) * a_boneWeights[i];
    }
    skin_vertex = vec4(skin_vertex.xyz, 1.0);

    vertex_out.color = a_color;
    vertex_out.normal = normalize(u_normalMatrix * skin_normal.xyz);
    vertex_out.tangent = normalize(u_normalMatrix * skin_tangent.xyz);
    vertex_out.texCoord = u_textureMatrix * a_texCoord;
    vertex_out.eyeVec = (u_modelViewMatrix * skin_vertex).xyz;
    gl_Position = u_modelViewProjectionMatrix * skin_vertex;
}
