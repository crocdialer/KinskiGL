#version 150 core

uniform int u_numTextures;
uniform sampler2D u_textureMap[16];
uniform vec3 u_lightDir;

uniform struct
{
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    vec4 emission;
    float shinyness;

} u_material;

in vec3 v_normal;
in vec4 v_texCoord;

out vec4 fragData;

void main()
{
    vec4 texColors = vec4(1);
    for(int i = 0; i < u_numTextures; i++)
    {
        texColors *= texture(u_textureMap[i], v_texCoord.st);
    }
    float nDotL = max(0.0, dot(v_normal, normalize(-u_lightDir)));
    fragData = u_material.diffuse * texColors * vec4(vec3(nDotL), 1.0f);
}

