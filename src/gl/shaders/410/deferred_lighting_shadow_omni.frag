#version 410

#define PI 3.1415926535897932384626433832795
#define MAX_NUM_LIGHTS 512
#define EPSILON 0.0010

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
uniform sampler2D u_sampler_2D[5];

// shadow cubemap
uniform samplerCube u_sampler_cube[1];

uniform vec2 u_clip_planes;
uniform mat4 u_camera_transform;
uniform float u_poisson_radius = 0.005;

#define ALBEDO 0
#define NORMAL 1
#define POSITION 2
#define EMISSION 3
#define MATERIAL_PROPS 4

const int NUM_TAPS = 12;
vec3 fTaps_Poisson[NUM_TAPS] = vec3[]
(
    vec3(-0.259506, 0.522649, -0.755241),
    vec3(0.775574, -0.378524, -0.496578),
    vec3(-0.346820, -0.630142, 0.317016),
    vec3(0.918634, -0.191248, 0.138310),
    vec3(0.205930, -0.096179, 0.823265),
    vec3(0.094824, -0.109210, 0.673721),
    vec3(-0.210247, 0.070444, -0.775253),
    vec3(0.687831, -0.035760, 0.043249),
    vec3(-0.006223, 0.941999, -0.253711),
    vec3(0.455131, -0.078537, -0.778169),
    vec3(0.559971, -0.629417, 0.327985),
    vec3(0.540030, 0.125809, -0.742234)
);

float nrand( vec2 n )
{
	return fract(sin(dot(n.xy, vec2(12.9898, 78.233)))* 43758.5453);
}

vec2 rot2d( vec2 p, float a )
{
	vec2 sc = vec2(sin(a),cos(a));
	return vec2( dot( p, vec2(sc.y, -sc.x) ), dot( p, sc.xy ) );
}

float shadow_factor(in samplerCube shadow_cube, in vec3 eye_space_pos, in vec3 light_pos)
{
    const float bias = 0.05;
    vec3 world_space_dir = mat3(u_camera_transform) * (eye_space_pos - light_pos);
    float norm_depth = (length(world_space_dir) - u_clip_planes.x) / (u_clip_planes.y - u_clip_planes.x);
    world_space_dir = normalize(world_space_dir);
    float sum = 0.0;
    float rnd = 6.28 * nrand(world_space_dir.xy);

    for(int i = 0; i < NUM_TAPS; ++i)
    {
        vec3 offset = fTaps_Poisson[i];
        offset.xz = rot2d(fTaps_Poisson[i].xz, rnd);
        float depth = texture(shadow_cube, world_space_dir + offset * u_poisson_radius).x + EPSILON;
        sum += depth < norm_depth ? 0 : 1;
    }
    return sum / NUM_TAPS;
}

// Microfacet specular = D*G*F / (4*NoL*NoV) = D*Vis*F
// Vis = G / (4*NoL*NoV)

// float D_blinn(float NoH, float shinyness)
// {
//     return pow(NoH, 4 * shinyness);
// }

vec3 F_schlick(vec3 f0, float u)
{
    return f0 + (vec3(1.0) - f0) * pow(1.0 - u, 5.0);
}

float Vis_schlick(float ndotl, float ndotv, float roughness)
{
	// = G_Schlick / (4 * ndotv * ndotl)
	float a = roughness + 1.0;
	float k = a * a * 0.125;

	float Vis_SchlickV = ndotv * (1 - k) + k;
	float Vis_SchlickL = ndotl * (1 - k) + k;

	return 0.25 / (Vis_SchlickV * Vis_SchlickL);
}

float D_GGX(float ndoth, float roughness)
{
	float m = roughness * roughness;
	float m2 = m * m;
	float d = (ndoth * m2 - ndoth) * ndoth + 1.0;
	return m2 / max(PI * d * d, 1e-8);
}

vec4 shade(in Lightsource light, in vec3 normal, in vec3 eyeVec, in vec4 base_color,
           in vec4 the_params, float shade_factor)
{
  vec3 lightDir = light.position - eyeVec;
  vec3 L = normalize(lightDir);
  vec3 E = normalize(-eyeVec);
  // vec3 R = reflect(-L, normal);
  vec3 H = normalize(L + E);
  vec3 ambient = /*mat.ambient */ light.ambient.rgb;
  float nDotL = max(0.f, dot(normal, L));
  float nDotH = max(0.f, dot(normal, H));
  float nDotV = max(0.f, dot(normal, E));
  float lDotH = max(0.f, dot(L, H));

  // distance^2
  float att = shade_factor;
  float dist2 = dot(lightDir, lightDir);
  float v = clamp(1.f - pow(dist2 / (light.radius * light.radius), 2.f), 0.f, 1.f);
  att *= min(1.f, light.intensity * v * v / (1.f + dist2 * light.quadraticAttenuation));

  // brdf term
  vec3 f0 = mix(vec3(0.04), base_color.rgb, the_params.x);
  vec3 F = F_schlick(f0, lDotH);
  float D = D_GGX(nDotH, the_params.y);
  float Vis = Vis_schlick(nDotL, nDotV, the_params.y);

  vec3 specular = att * F * D * Vis;
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
    // vec4 specular = texture(u_sampler_2D[SPECULAR], tex_coord);
    vec4 mat_prop = texture(u_sampler_2D[MATERIAL_PROPS], tex_coord);
    bool receive_shadow = bool(mat_prop.b);
    const float min_shade = 0.1, max_shade = 1.0;
    float sf = receive_shadow ?
        shadow_factor(u_sampler_cube[0], position, u_lights[u_light_index].position) : 1.0;
    sf = mix(min_shade, max_shade, sf);
    fragData = shade(u_lights[u_light_index], normal, position, color, mat_prop, sf);
}
