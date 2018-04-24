#version 410

uniform samplerCube u_sampler_cube[1];

// struct Material
// {
//     vec4 diffuse;
//     vec4 ambient;
//     vec4 emission;
//     vec4 point_vals;// (size, constant_att, linear_att, quad_att)
//     float metalness;
//     float roughness;
//     int shadow_properties;
//     int texture_properties;
// };
//
// layout(std140) uniform MaterialBlock
// {
//     Material u_material;
// };

in VertexData
{
    vec3 eyeVec;
} vertex_in;

out vec4 fragData;

void main()
{
    fragData = texture(u_sampler_cube[0], vertex_in.eyeVec);
}
