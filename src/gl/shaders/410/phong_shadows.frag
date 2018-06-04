#version 410 core
#extension GL_ARB_separate_shader_objects : enable

#define PI 3.1415926535897932384626433832795
#define ONE_OVER_PI	0.318309886
#define NUM_SHADOW_LIGHTS 4
#define EPSILON 0.00001

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

struct Lightsource
{
    vec3 position;
    int type;
    vec4 diffuse;
    vec4 ambient;
    vec3 direction;
    float intensity;
    float radius;
    float spotCosCutoff;
    float spotExponent;
    float quadraticAttenuation;
};

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

vec3 projected_coords(in vec4 the_lightspace_pos)
{
    vec3 proj_coords = the_lightspace_pos.xyz / the_lightspace_pos.w;
    proj_coords = (vec3(1) + proj_coords) * 0.5;
    return proj_coords;
}

vec4 BRDF_Lambertian(vec4 color, float metalness)
{
	color.rgb = mix(color.rgb, vec3(0.0), metalness);
	color.rgb *= ONE_OVER_PI;
	return color;
}

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
    vec3 lightDir = light.type > 0 ? (light.position - eyeVec) : -light.direction;
    vec3 L = normalize(lightDir);
    vec3 E = normalize(-eyeVec);
    vec3 H = normalize(L + E);
    // vec3 R = reflect(-L, normal);

    vec3 ambient = /*mat.ambient */ light.ambient.rgb;
    float nDotL = max(0.f, dot(normal, L));
    float nDotH = max(0.f, dot(normal, H));
    float nDotV = max(0.f, dot(normal, E));
    float lDotH = max(0.f, dot(L, H));
    float att = shade_factor;

    if(light.type > 0)
    {
        // distance^2
        float dist2 = dot(lightDir, lightDir);
        float v = clamp(1.f - pow(dist2 / (light.radius * light.radius), 2.f), 0.f, 1.f);
        att *= light.intensity * v * v / (1.f + dist2 * light.quadraticAttenuation);

        if(light.type > 1)
        {
            float spot_effect = dot(normalize(light.direction), -L);
            att *= spot_effect < light.spotCosCutoff ? 0 : 1;
            spot_effect = pow(spot_effect, light.spotExponent);
            att *= spot_effect;
        }
    }

    // brdf term
    vec3 f0 = mix(vec3(0.04), base_color.rgb, the_params.x) * light.diffuse.rgb;
    vec3 F = F_schlick(f0, lDotH);
    float D = D_GGX(nDotH, the_params.y);
    float Vis = Vis_schlick(nDotL, nDotV, the_params.y);

    vec3 specular = F * D * Vis;
    vec4 diffuse = BRDF_Lambertian(base_color, the_params.x) * light.diffuse;
    return vec4(ambient, 1.0) + vec4(diffuse.rgb + specular, diffuse.a) * att * nDotL;
}

//uniform Material u_material;
layout(std140) uniform MaterialBlock
{
  Material u_material;
};

layout(std140) uniform LightBlock
{
  int u_numLights;
  Lightsource u_lights[16];
};

// regular textures
uniform int u_numTextures;
uniform sampler2D u_sampler_2D[4];

uniform sampler2D u_shadow_map[NUM_SHADOW_LIGHTS];
uniform vec2 u_shadow_map_size = vec2(1024);
uniform float u_poisson_radius = 3.0;

in VertexData
{
  vec4 color;
  vec4 texCoord;
  vec3 normal;
  vec3 eyeVec;
  vec4 lightspace_pos[NUM_SHADOW_LIGHTS];
} vertex_in;

out vec4 fragData;

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

	vec4 basis = vec4( rot2d(vec2(1,0),rnd), rot2d(vec2(0,1),rnd) );

	for(int i = 0; i < NUM_TAPS; i++)
	{
		// vec2 ofs = vec2(dot(fTaps_Poisson[i],basis.xz),
    //                 dot(fTaps_Poisson[i],basis.yw));
		vec2 ofs = rot2d( fTaps_Poisson[i], rnd );
		vec2 texcoord = light_space_pos.xy + u_poisson_radius * ofs / u_shadow_map_size;
    float depth = texture(shadow_map, texcoord).x;
    bool is_in_shadow = depth < (light_space_pos.z - EPSILON);
    factor += is_in_shadow ? 0 : 1;
  }
  return factor / NUM_TAPS;
}

void main()
{
  vec4 texColors = vertex_in.color * u_material.diffuse;

  if(u_numTextures > 0)
    texColors *= texture(u_sampler_2D[0], vertex_in.texCoord.st);

  if(smoothstep(0.0, 1.0, texColors.a) < 0.01){ discard; }

  vec3 normal = normalize(vertex_in.normal);
  vec4 shade_color = vec4(0);

  float factor[NUM_SHADOW_LIGHTS];

  float min_shade = 0.1, max_shade = 1.0;

  for(int i = 0; i < NUM_SHADOW_LIGHTS; i++)
  {
    factor[i] = shadow_factor(u_shadow_map[i], projected_coords(vertex_in.lightspace_pos[i]));
    factor[i] = mix(min_shade, max_shade, factor[i]);
  }

  int num_lights = min(NUM_SHADOW_LIGHTS, u_numLights);

  for(int i = 0; i < num_lights; i++)
    shade_color += shade(u_lights[i], normal, vertex_in.eyeVec, texColors,
                         vec4(u_material.metalness, u_material.roughness, 0, 1), factor[i]);

  fragData = shade_color;
}
