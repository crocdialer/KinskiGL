#version 410

uniform int u_numTextures;
uniform sampler2D u_sampler_2D[2];

#define COLOR 0
#define DEPTH 1

struct Material
{
    vec4 diffuse;
    vec4 ambient;
    vec4 emission;
    vec4 point_vals;// (size, constant_att, linear_att, quad_att)
    float metalness;
    float roughness;
    int shadow_properties;
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

float linear_depth(float val)
{
    float zNear = 0.5;    // TODO: Replace by the zNear of your perspective projection
    float zFar  = 2000.0; // TODO: Replace by the zFar  of your perspective projection
    return (2.0 * zNear) / (zFar + zNear - val * (zFar - zNear));
}

void main()
{
    vec4 texColors = texture(u_sampler_2D[COLOR], vertex_in.texCoord.st);
    float depth = texture(u_sampler_2D[DEPTH], vertex_in.texCoord.st).x;
    gl_FragDepth = depth;
    fragData = u_material.diffuse * texColors;
}
