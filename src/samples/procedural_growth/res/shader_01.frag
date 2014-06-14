#version 150 core
uniform int u_numTextures;
uniform sampler2D u_textureMap[16];
uniform struct{
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    vec4 emission;
} u_material;

in VertexData{
    vec3 normal;
    vec4 color;
    vec2 texCoord;
} vertex_in;

out vec4 fragData;
void main()
{
    vec4 texColors = vertex_in.color;
    for(int i = 0; i < u_numTextures; i++)
    {
      texColors *= texture(u_textureMap[i], vertex_in.texCoord.st);
    }
    fragData = u_material.diffuse * texColors;
}
