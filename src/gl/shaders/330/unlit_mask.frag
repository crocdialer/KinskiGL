#version 330

uniform int u_numTextures;
uniform sampler2D u_sampler_2D[2];

#define COLORMAP 0
#define MASK 1

struct Material
{
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    vec4 emission;
    vec4 point_vals;// (size, constant_att, linear_att, quad_att)
    float shinyness;
};

layout(std140) uniform MaterialBlock
{
    Material u_material;
};

in VertexData
{
    vec4 color;
    vec2 texCoord;
} vertex_in;

out vec4 fragData;

void main()
{
    vec4 texColors = vertex_in.color;
    if(u_numTextures > 0)
        texColors *= texture(u_sampler_2D[COLORMAP], vertex_in.texCoord.st);
    float mask = texture(u_sampler_2D[MASK], vertex_in.texCoord.st).x;
    texColors.a *= mask;
    fragData = u_material.diffuse * texColors;
}
