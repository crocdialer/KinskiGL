#version 150 core

uniform mat4 u_modelViewMatrix;
uniform mat4 u_modelViewProjectionMatrix;
uniform mat3 u_normalMatrix;
uniform mat4 u_textureMatrix;
uniform mat4 u_bones[25];

in vec4 a_vertex;
in vec4 a_texCoord;
in vec3 a_normal;
in ivec4 a_boneIds;
in vec4 a_boneWeights;

out vec4 v_texCoord;
out vec3 v_normal;

void main()
{
    vec4 newVertex = vec4(0);
    vec4 newNormal = vec4(0);
    
    for (int i = 0; i < 4; i++)
    {
        newVertex += u_bones[a_boneIds[i]] * a_vertex * a_boneWeights[i];
        newNormal += u_bones[a_boneIds[i]] * vec4(a_normal, 0.0) * a_boneWeights[i];
    }
    
    v_normal = normalize(u_normalMatrix * newNormal.xyz);
    v_texCoord =  u_textureMatrix * a_texCoord;
    gl_Position = u_modelViewProjectionMatrix * vec4(newVertex.xyz, 1.0);
}

