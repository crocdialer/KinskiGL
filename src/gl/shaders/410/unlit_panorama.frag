#version 410 core
#extension GL_ARB_separate_shader_objects : enable

#define ONE_OVER_PI 0.31830988618379067153776752674503

uniform sampler2D u_sampler_2D[1];
#define COLOR 0

struct Material
{
    vec4 diffuse;
    vec4 ambient;
    vec4 emission;
    vec4 point_vals;// (size, constant_att, linear_att, quad_att)
    float metalness;
    float roughness;
    float occlusion;
    int shadow_properties;
    int texture_properties;
};

layout(std140) uniform MaterialBlock
{
    Material u_material;
};

in VertexData
{
    vec3 eyeVec;
} vertex_in;

out vec4 fragData;

// map normalized direction to equirectangular texture coordinate
vec2 panorama(vec3 ray)
{
	return vec2(0.5 + 0.5 * atan(ray.x, -ray.z) * ONE_OVER_PI, acos(ray.y) * ONE_OVER_PI);
}

void main()
{
    fragData = texture(u_sampler_2D[COLOR], panorama(normalize(vertex_in.eyeVec)));
}
