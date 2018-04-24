#version 410

#define PI 3.1415926535897932384626433832795
#define ONE_OVER_PI 0.31830988618379067153776752674503

uniform mat4 u_camera_transform;

#define ALBEDO 0
#define NORMAL 1
#define POSITION 2
#define EMISSION 3
#define MATERIAL_PROPS 4
uniform sampler2D u_sampler_2D[5];

#define ENV_DIFFUSE 0
#define ENV_SPEC 1
uniform samplerCube u_sampler_cube[2];

vec4 sample_diffuse(in samplerCube diff_map, in vec3 normal)
{
    return texture(diff_map, normal) * ONE_OVER_PI;
}

vec4 sample_reflection(in samplerCube env_map, in vec3 normal, in vec3 eyeVec, in vec4 base_color,
                       float roughness)
{
    vec3 sample_dir = mat3(u_camera_transform) * reflect(eyeVec, normal);
    return (1 - roughness) * texture(env_map, sample_dir);
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
    fragData = color * sample_diffuse(u_sampler_cube[ENV_DIFFUSE], normal) +
        0.15 * sample_reflection(u_sampler_cube[ENV_SPEC], normal, position, color, mat_prop.y);
}
