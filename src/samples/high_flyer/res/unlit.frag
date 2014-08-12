#version 330

uniform int u_numTextures;
uniform sampler2D u_textureMap[16];
uniform struct Material
{
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    vec4 emission;
    float shinyness;
} u_material;

in VertexData{
    vec4 color;
    vec4 texCoord;
    vec3 normal;
    vec3 eyeVec;
} vertex_in;
          
out vec4 fragData;
void main()
{
    vec4 texColors = vertex_in.color;
    
    texColors *= u_numTextures > 0 ? texture(u_textureMap[0], vertex_in.texCoord.st) : vec4(1);
    texColors *= u_numTextures > 1 ? texture(u_textureMap[1], vertex_in.texCoord.st) : vec4(1);
    texColors *= u_numTextures > 2 ? texture(u_textureMap[2], vertex_in.texCoord.st) : vec4(1);
    
    fragData = u_material.diffuse * texColors;
}
