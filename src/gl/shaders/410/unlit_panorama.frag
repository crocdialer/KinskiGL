#version 410

#define ONE_OVER_PI	0.3183098861837907

uniform sampler2D u_sampler_2D[1];

struct Material
{
    vec4 diffuse;
    vec4 ambient;
    vec4 emission;
    vec4 point_vals;// (size, constant_att, linear_att, quad_att)
    float metalness;
    float roughness;
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

// map normalized direction to panorama texture coordinate
vec2 panorama(vec3 ray)
{
	return vec2(0.5 + 0.5 * atan(ray.x, -ray.z) * ONE_OVER_PI, acos(ray.y) * ONE_OVER_PI);
}

void main()
{
    fragData = texture(u_sampler_2D[0], panorama(normalize(vertex_in.eyeVec)));
}
