#version 150 core

uniform int u_numTextures;
uniform sampler2D u_textureMap[16];

uniform struct
{
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    vec4 emission;

} u_material;

in vec4 v_texCoord;
out vec4 fragData;

void main()
{
    vec4 texColors = vec4(1);
    for(int i = 0; i < u_numTextures; i++)
    {
        texColors *= texture(u_textureMap[i], v_texCoord.st);
    }
    fragData = u_material.diffuse * texColors;
}

