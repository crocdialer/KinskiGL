#version 410

// window dimension
uniform vec2 u_window_dimension;
uniform mat4 u_camera_transform;

// regular textures
uniform int u_numTextures;
uniform sampler2D u_sampler_2D[5];
uniform samplerCube u_sampler_cube[1];

#define ALBEDO 0
#define NORMAL 1
#define POSITION 2
#define EMISSION 3
#define MATERIAL_PROPS 4

vec4 sample_enviroment(in samplerCube env_map, in vec3 normal, in vec3 eyeVec, in vec4 base_color,
                       in vec4 the_params)
{
    float roughness = the_params.y;
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
    vec2 tex_coord = gl_FragCoord.xy / u_window_dimension;
    vec4 color = texture(u_sampler_2D[ALBEDO], tex_coord);
    vec3 normal = normalize(texture(u_sampler_2D[NORMAL], tex_coord).xyz);
    vec3 position = texture(u_sampler_2D[POSITION], tex_coord).xyz;
    vec4 mat_prop = texture(u_sampler_2D[MATERIAL_PROPS], tex_coord);
    fragData = 0.15 * sample_enviroment(u_sampler_cube[0], normal, position, color, mat_prop);
}
