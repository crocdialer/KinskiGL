precision mediump float;
precision lowp int;

#define ONE_OVER_PI 0.31830988618379067153776752674503

uniform sampler2D u_sampler_2D[1];
#define COLOR 0

struct Material
{
    vec4 diffuse;
    vec4 emission;
    vec4 point_vals;// (size, constant_att, linear_att, quad_att)
    float metalness;
    float roughness;
    float occlusion;
    int shadow_properties;
    int texture_properties;
};
uniform Material u_material;

varying vec3 v_eyeVec;

// map normalized direction to equirectangular texture coordinate
vec2 panorama(vec3 ray)
{
	return vec2(0.5 + 0.5 * atan(ray.x, -ray.z) * ONE_OVER_PI, acos(ray.y) * ONE_OVER_PI);
}

void main(void)
{
    gl_FragColor = texture2D(u_sampler_2D[COLOR], panorama(normalize(v_eyeVec)));
}
