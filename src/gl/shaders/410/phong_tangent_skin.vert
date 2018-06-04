#version 410 core
#extension GL_ARB_separate_shader_objects : enable

uniform mat4 u_modelViewMatrix;
uniform mat4 u_modelViewProjectionMatrix;
uniform mat3 u_normalMatrix;
uniform mat4 u_textureMatrix;
uniform mat4 u_bones[110];

layout(location = 0) in vec4 a_vertex;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_texCoord;
// layout(location = 3) in vec4 a_color;
layout(location = 5) in vec3 a_tangent;
layout(location = 6) in ivec4 a_boneIds;
layout(location = 7) in vec4 a_boneWeights;

out VertexData
{
  // vec4 color;
  vec4 texCoord;
  vec3 normal;
  vec3 eyeVec;
  vec3 tangent;
} vertex_out;

void main()
{
    vec4 newVertex = vec4(0);
    vec4 newNormal = vec4(0);
    vec4 newTangent = vec4(0);

    for (int i = 0; i < 4; i++)
    {
        newVertex += u_bones[a_boneIds[i]] * a_vertex * a_boneWeights[i];
        newNormal += u_bones[a_boneIds[i]] * vec4(a_normal, 0.0) * a_boneWeights[i];
        newTangent += u_bones[a_boneIds[i]] * vec4(a_tangent, 0.0) * a_boneWeights[i];
    }
    newVertex = vec4(newVertex.xyz, 1.0);

    // vertex_out.color = a_color;
    vertex_out.normal = normalize(u_normalMatrix * newNormal.xyz);
    vertex_out.tangent = normalize(u_normalMatrix * newTangent.xyz);
    vertex_out.texCoord = u_textureMatrix * a_texCoord;
    vertex_out.eyeVec = (u_modelViewMatrix * newVertex).xyz;
    gl_Position = u_modelViewProjectionMatrix * newVertex;
}
