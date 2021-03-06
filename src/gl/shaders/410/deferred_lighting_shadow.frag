// #version 410
// #include brdf.glsl

#define MAX_NUM_LIGHTS 512
#define EPSILON 0.00001

layout(std140) uniform LightBlock
{
    int u_numLights;
    Lightsource u_lights[MAX_NUM_LIGHTS];
};

// window dimension
uniform vec2 u_window_dimension;
uniform int u_light_index;

// regular textures
uniform int u_numTextures;
uniform sampler2D u_sampler_2D[6];

uniform mat4 u_shadow_matrix;
uniform vec2 u_shadow_map_size = vec2(1024);
uniform float u_poisson_radius = 3.0;

#define ALBEDO 0
#define NORMAL 1
#define POSITION 2
#define EMISSION 3
#define AO_ROUGH_METAL 4
#define SHADOW_MAP 5

const int NUM_TAPS = 12;
vec2 fTaps_Poisson[NUM_TAPS] = vec2[]
(
    vec2(-.326,-.406),
	vec2(-.840,-.074),
	vec2(-.696, .457),
	vec2(-.203, .621),
	vec2( .962,-.195),
	vec2( .473,-.480),
	vec2( .519, .767),
	vec2( .185,-.893),
	vec2( .507, .064),
	vec2( .896, .412),
	vec2(-.322,-.933),
	vec2(-.792,-.598)
);

vec3 shadow_coords(in vec3 the_eye_space_coord)
{
    vec4 light_space_pos = u_shadow_matrix * vec4(the_eye_space_coord, 1.0);
    vec3 proj_coords = light_space_pos.xyz / light_space_pos.w;
    proj_coords = (vec3(1) + proj_coords) * 0.5;
    return proj_coords;
}

float nrand( vec2 n )
{
	return fract(sin(dot(n.xy, vec2(12.9898, 78.233)))* 43758.5453);
}

vec2 rot2d( vec2 p, float a )
{
	vec2 sc = vec2(sin(a),cos(a));
	return vec2( dot( p, vec2(sc.y, -sc.x) ), dot( p, sc.xy ) );
}

float shadow_factor(in sampler2D shadow_map, in vec3 light_space_pos)
{
    float rnd = 6.28 * nrand(light_space_pos.xy);
    float factor = 0.0;

    for(int i = 0; i < NUM_TAPS; i++)
    {
    	vec2 ofs = rot2d(fTaps_Poisson[i], rnd);
    	vec2 texcoord = light_space_pos.xy + u_poisson_radius * ofs / u_shadow_map_size;
        float depth = texture(shadow_map, texcoord).x;
        bool is_in_shadow = depth < (light_space_pos.z - EPSILON);
        factor += is_in_shadow ? 0 : 1;
    }
    return factor / NUM_TAPS;
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
    vec3 ao_rough_metal = texture(u_sampler_2D[AO_ROUGH_METAL], tex_coord).rgb;
    const float min_shade = 0.1, max_shade = 1.0;
    float shadow_factor = shadow_factor(u_sampler_2D[SHADOW_MAP], shadow_coords(position));
    shadow_factor = mix(min_shade, max_shade, shadow_factor);
    fragData = shade(u_lights[u_light_index], normal, position, color, ao_rough_metal.g, ao_rough_metal.b, shadow_factor);
}
