#version 410

#define MAX_NUM_LIGHTS 512
// #define EPSILON 0.00005
#define EPSILON 0.00001

struct Lightsource
{
    vec3 position;
    int type;
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    vec3 direction;
    float intensity;
    float radius;
    float spotCosCutoff;
    float spotExponent;
    // float constantAttenuation;
    // float linearAttenuation;
    float quadraticAttenuation;
};

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
uniform sampler2D u_sampler_2D[7];

uniform mat4 u_shadow_matrix;
uniform vec2 u_shadow_map_size = vec2(1024);
uniform float u_poisson_radius = 3.0;

#define ALBEDO 0
#define NORMAL 1
#define POSITION 2
#define EMISSION 3
#define SPECULAR 4
#define MATERIAL_PROPS 5
#define SHADOW_MAP 6

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

float D_blinn(float NoH, float shinyness)
{
    return pow(NoH, 4 * shinyness);
}

vec3 F_schlick(in vec3 c_spec, float NoL)
{
    return c_spec + (1 - c_spec) * pow(1 - NoL, 5);
}

vec4 shade(in Lightsource light, in vec3 normal, in vec3 eyeVec, in vec4 base_color,
           in vec4 the_spec, float shade_factor)
{
    vec3 lightDir = light.type > 0 ? (light.position - eyeVec) : -light.direction;
    vec3 L = normalize(lightDir);
    vec3 E = normalize(-eyeVec);
    vec3 H = normalize(L + E);
    // vec3 R = reflect(-L, normal);

    vec3 ambient = /*mat.ambient */ light.ambient.rgb;
    float nDotL = max(0.f, dot(normal, L));
    float nDotH = max(0.f, dot(normal, H));
    float att = shade_factor;

    if(light.type > 0)
    {
        // distance^2
        float dist2 = dot(lightDir, lightDir);
        float v = clamp(1.f - pow(dist2 / (light.radius * light.radius), 2.f), 0.f, 1.f);
        att *= min(1.f, light.intensity * v * v / (1.f + dist2 * light.quadraticAttenuation));

        if(light.type > 1)
        {
            float spot_effect = dot(normalize(light.direction), -L);
            att *= spot_effect < light.spotCosCutoff ? 0 : 1;
            spot_effect = pow(spot_effect, light.spotExponent);
            att *= spot_effect;
        }
    }

    // brdf term
    vec3 specular = att * light.specular.rgb * F_schlick(the_spec.rgb, nDotL) * D_blinn(nDotH, the_spec.a);
    vec3 diffuse = (1 - specular) * att * vec3(nDotL) * light.diffuse.rgb;
    return base_color * vec4(ambient + diffuse, 1.0) + vec4(specular, 0);
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
    vec4 specular = texture(u_sampler_2D[SPECULAR], tex_coord);
    vec4 mat_prop = texture(u_sampler_2D[MATERIAL_PROPS], tex_coord);
    specular.a = mat_prop.g;
    bool receive_shadow = bool(mat_prop.b);

    const float min_shade = 0.1, max_shade = 1.0;
    float shadow_factor = receive_shadow ? shadow_factor(u_sampler_2D[SHADOW_MAP], shadow_coords(position)) : 1.0;
    shadow_factor = mix(min_shade, max_shade, shadow_factor);
    fragData = shade(u_lights[u_light_index], normal, position, color, specular, shadow_factor);
}
