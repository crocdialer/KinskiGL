#version 410

struct Material
{
  vec4 diffuse;
  vec4 ambient;
  vec4 specular;
  vec4 emission;
  float shinyness;
};

uniform int u_numTextures;
uniform sampler2D u_textureMap[8];
uniform Material u_material;

in VertexData 
{ 
  vec4 color;
} vertex_in;

out vec4 fragData;
void main()
{
    vec4 texColors = vertex_in.color; 
    for(int i = 0; i < u_numTextures; i++)
    {
        texColors *= texture(u_textureMap[i], gl_PointCoord.xy);
    }
    fragData = u_material.diffuse * texColors;
}
