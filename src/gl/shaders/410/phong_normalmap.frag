#version 410 core
#extension GL_ARB_separate_shader_objects : enable

#define MAX_NUM_LIGHTS 8

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

layout(std140) uniform MaterialBlock
{
  Material u_material;
};

layout(std140) uniform LightBlock
{
  int u_numLights;
  Lightsource u_lights[MAX_NUM_LIGHTS];
};

// regular textures
uniform int u_numTextures;
uniform sampler2D u_sampler_2D[4];

in VertexData
{
  vec4 color;
  vec4 texCoord;
  vec3 eyeVec;
  vec3 light_position[MAX_NUM_LIGHTS];
  vec3 light_direction[MAX_NUM_LIGHTS];
} vertex_in;

out vec4 fragData;

#define DIFFUSE 0
#define NORMAL 1

vec3 normalFromHeightMap(sampler2D theMap, vec2 theCoords, float theStrength)
{
  float center = texture(theMap, theCoords).r;
  float U = texture(theMap, theCoords + vec2( 0.005, 0)).r;
  float V = texture(theMap, theCoords + vec2(0, 0.005)).r;
  float dHdU = U - center;
  float dHdV = V - center;
  vec3 normal = vec3( -dHdU, dHdV, 0.05 / theStrength);
  return normalize(normal);
}

void main()
{
  vec4 texColors = /*vertex_in.color **/ texture(u_sampler_2D[DIFFUSE], vertex_in.texCoord.st);
  if(smoothstep(0.0, 1.0, texColors.a) < 0.01){ discard; }

  vec3 normal;
  //normal = normalFromHeightMap(u_sampler_2D[1], vertex_in.texCoord.xy, 0.8);

  normal = normalize(2.0 * (texture(u_sampler_2D[NORMAL], vertex_in.texCoord.xy).xyz - vec3(0.5)));
  vec4 shade_color = vec4(0, 0, 0, 1);
  int num_lights = min(u_numLights, MAX_NUM_LIGHTS);

  for(int i = 0; i < num_lights; i++)
  {
    shade_color += shade(u_lights[i], normal, vertex_in.eyeVec, texColors,
                         vec4(u_material.metalness, u_material.roughness, 0, 1), 1.0));
  }
  fragData = shade_color;
}
