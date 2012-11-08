#version 150 core

uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;

in vec4 a_vertex;
in vec4 a_texCoord;
in vec4 a_normal;

out vec4 v_texCoord;
out vec4 v_normal;

void main()
{
//    v_normal = normalize(u_normalMatrix * a_normal);
//    nDotL = max(0.0, dot(v_normal, normalize(-u_lightDir)));

    v_texCoord =  u_textureMatrix * a_texCoord;
    gl_Position = u_modelViewProjectionMatrix * a_vertex;
}

