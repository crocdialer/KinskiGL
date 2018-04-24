#version 410

#define PI 3.1415926535897932384626433832795
#define ONE_OVER_PI 0.31830988618379067153776752674503

uniform mat4 u_camera_transform;

#define ALBEDO 0
#define NORMAL 1
#define POSITION 2
#define EMISSION 3
#define MATERIAL_PROPS 4
#define BRDF_LUT 5
uniform sampler2D u_sampler_2D[6];

#define ENV_DIFFUSE 0
#define ENV_SPEC 1
uniform samplerCube u_sampler_cube[2];

vec3 sample_diffuse(in samplerCube diff_map, in vec3 normal)
{
    return texture(diff_map, normal).rgb * ONE_OVER_PI;
}

vec3 sample_reflection(in samplerCube env_map, in vec3 normal, in vec3 eyeVec, float roughness)
{
    vec3 sample_dir = mat3(u_camera_transform) * reflect(eyeVec, normal);
    return 0.15 * (1 - roughness) * texture(env_map, sample_dir).rgb;
}

vec3 compute_enviroment_lighting(vec3 position, vec3 normal, vec3 albedo, float roughness,
                                 float metalness, float aoVal)
{
	vec3 v = normalize(position);
	// vec3 r = normalize(reflect(-v, normal));

	vec3 diffIr = sample_diffuse(u_sampler_cube[ENV_DIFFUSE], normal);

	float specMipLevel = 0;//roughness * float(pcs.totalMipLevels - 1);

    vec3 specIr = sample_reflection(u_sampler_cube[ENV_SPEC], normal, position, roughness);
	// vec3 specIr = textureLod(u_sampler_cube[ENV_SPEC], r, specMipLevel).rgb;
	// float NoV = clamp(dot(normal, v), 0.0, 1.0);
	// vec2 brdfTerm = textureLod(brdfLuts[0], vec2(NoV, clamp(roughness, 0.0, 1.0)), 0).rg;

	const vec3 dielectricF0 = vec3(0.04);
	vec3 diffColor = albedo * (1.0 - metalness); // if it is metal, no diffuse color
	vec3 specColor = mix(dielectricF0, albedo, metalness); // since metal has no albedo, we use the space to store its F0

	vec3 distEnvLighting = diffColor * diffIr + specIr /* * (specColor * brdfTerm.x + brdfTerm.y)*/;
	// distEnvLighting *= aoVal * distEnvLightStrength;

	return distEnvLighting;
}

in VertexData
{
    vec4 color;
    vec2 texCoord;
} vertex_in;

out vec4 fragData;

void main()
{
    vec2 tex_coord = gl_FragCoord.xy / textureSize(u_sampler_2D[ALBEDO], 0);
    vec4 color = texture(u_sampler_2D[ALBEDO], tex_coord);
    vec3 normal = normalize(texture(u_sampler_2D[NORMAL], tex_coord).xyz);
    vec3 position = texture(u_sampler_2D[POSITION], tex_coord).xyz;
    vec4 mat_prop = texture(u_sampler_2D[MATERIAL_PROPS], tex_coord);
    fragData = vec4(compute_enviroment_lighting(position, normal, color.rgb, mat_prop.y, mat_prop.x, 1.0), 1.0);

    // fragData = color * sample_diffuse(u_sampler_cube[ENV_DIFFUSE], normal) +
    //     0.15 * sample_reflection(u_sampler_cube[ENV_SPEC], normal, position, color, mat_prop.y);
}
