#version 410

#define NUM_SHADOW_LIGHTS 4
// #define EPSILON 0.00010
#define EPSILON 0.00001

struct Material
{
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    vec4 emission;
    vec4 point_vals;// (size, constant_att, linear_att, quad_att)
    float shinyness;
    int shadow_properties;
};

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

vec4 shade(in Lightsource light, in vec3 normal, in vec3 eyeVec, in vec4 base_color,
           in vec4 the_spec, float shade_factor)
{
    vec3 lightDir = light.type > 0 ? (light.position - eyeVec) : -light.direction;
    vec3 L = normalize(lightDir);
    vec3 E = normalize(-eyeVec);
    vec3 H = normalize(L + E);
    // vec3 R = reflect(-L, normal);

    vec4 ambient = /*mat.ambient */ light.ambient;
    float att = 1.f;
    float nDotL = max(0.f, dot(normal, L));
    float nDotH = max(0.f, dot(normal, H));

    if(light.type > 0)
    {
        // distance^2
        float dist2 = dot(lightDir, lightDir);
        float v = clamp(1.f - pow(dist2 / (light.radius * light.radius), 2.f), 0.f, 1.f);
        att = min(1.f, light.intensity * v * v / (1.f + dist2 * light.quadraticAttenuation));

        if(light.type > 1)
        {
            float spotEffect = dot(normalize(light.direction), -L);

            if(spotEffect < light.spotCosCutoff)
            {
                att = 0.f;
                base_color * ambient;
            }
            spotEffect = pow(spotEffect, light.spotExponent);
            att *= spotEffect;
        }
    }

    // phong speculars
    // float specIntesity = clamp(pow(max(dot(R, E), 0.0), the_spec.a), 0.0, 1.0);

    // blinn-phong speculars
    float specIntesity = pow(nDotH, 4 * the_spec.a);

    vec4 diffuse = light.diffuse * vec4(att * shade_factor * vec3(nDotL), 1.0);
    vec3 final_spec = shade_factor * att * the_spec.rgb * light.specular.rgb * specIntesity;
    return base_color * (ambient + diffuse) + vec4(final_spec, 0);
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
  //float xOffset = 1.0/u_shadow_map_size.x;
  //float yOffset = 1.0/u_shadow_map_size.y;

  //for(int y = -2 ; y <= 2 ; y++)
  //{
  //  for (int x = -2 ; x <= 2 ; x++)
  //  {
  //    vec2 offset = vec2(x * xOffset, y * yOffset);
  //    float depth = texture(u_shadow_map[shadow_index], proj_coords.xy + offset).x;
  //    bool is_in_shadow = depth < (proj_coords.z - EPSILON);
  //    factor += is_in_shadow ? 0 : 1;
  //  }
  //}
  //return (0.5 + (factor / 50.0));
}

void main()
{


  vec4 texColors = vertex_in.color;//vec4(1);

  //for(int i = 0; i < u_numTextures; i++)
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
    shade_color += shade(u_lights[i], normal, vertex_in.eyeVec, texColors, u_material.specular, factor[i]);

  fragData = shade_color;
}
