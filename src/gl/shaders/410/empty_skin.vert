#version 410 core

uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_bones[110];

layout(location = 0) in vec4 a_vertex;
layout(location = 6) in ivec4 a_boneIds;
layout(location = 7) in vec4 a_boneWeights;

void main()
{
    vec4 newVertex = vec4(0);

    for (int i = 0; i < 4; i++)
    {
        newVertex += u_bones[a_boneIds[i]] * a_vertex * a_boneWeights[i];
    }
    gl_Position = u_modelViewProjectionMatrix * newVertex;
}
