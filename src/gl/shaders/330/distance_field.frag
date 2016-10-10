#version 330

uniform int u_numTextures;
uniform sampler2D u_sampler_2D[1];

uniform float u_buffer = 0.70;
uniform float u_gamma = 0.05;

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
    vec4 color = vertex_in.color * u_material.diffuse;
    float dist = texture(u_sampler_2D[0], vertex_in.texCoord.st).r;
    float alpha = smoothstep(u_buffer - u_gamma, u_buffer, dist);
    fragData = vec4(color.rgb, color.a * alpha);
}
