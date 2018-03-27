#version 410

#define PI 3.1415926535897932384626433832795
#define MAX_NUM_LIGHTS 8

struct Material
{
    vec4 diffuse;
    vec4 ambient;
    vec4 emission;
    vec4 point_vals;// (size, constant_att, linear_att, quad_att)
    float metalness;
    float roughness;
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

vec4 BRDF_Lambertian(vec4 color, float metalness)
{
	color.rgb = mix(color.rgb, vec3(0.0), metalness);
	// color.rgb *= ONE_OVER_PI;
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
  vec3 f0 = mix(vec3(0.04), base_color.rgb * light.diffuse.rgb, the_params.x);
  vec3 F = F_schlick(f0, lDotH);
  float D = D_GGX(nDotH, the_params.y);
  float Vis = Vis_schlick(nDotL, nDotV, the_params.y);

  vec3 specular = F * D * Vis;
  vec4 diffuse = BRDF_Lambertian(base_color, the_params.x) * light.diffuse;
  return vec4(ambient, 1.0) + vec4(diffuse.rgb + specular, diffuse.a) * att * nDotL;
}

uniform int u_numTextures;
uniform sampler2D u_sampler_2D[4];

layout(std140) uniform MaterialBlock
{
  Material u_material;
};

layout(std140) uniform LightBlock
{
  int u_numLights;
  Lightsource u_lights[MAX_NUM_LIGHTS];
};

in VertexData
{
  vec4 color;
  vec3 eyeVec;
  float point_size;
} vertex_in;

out vec4 fragData;

void main()
{
  vec4 texColors = vertex_in.color;

  //for(int i = 0; i < u_numTextures; i++)
  if(u_numTextures > 0)
  {
    texColors *= texture(u_sampler_2D[0], gl_PointCoord);
  }
  vec3 normal;
  normal.xy = gl_PointCoord * vec2(2.0, -2.0) + vec2(-1.0, 1.0);
  float mag = dot(normal.xy, normal.xy);

  if (mag > 1.0) discard;

  normal.z = sqrt(1.0 - mag);
  normalize(normal);
  vec3 spherePosEye = -(vertex_in.eyeVec + normal * vertex_in.point_size / 2.0);

  vec4 shade_color = vec4(0);

  int num_lights = min(MAX_NUM_LIGHTS, u_numLights);

  for(int i = 0; i < num_lights; ++i)
  {
      shade_color += shade(u_lights[i], normal, spherePosEye, texColors,
                           vec4(u_material.metalness, u_material.roughness, 0, 1), 1.0);
  }
  fragData = shade_color;//texColors * (u_material.diffuse * vec4(vec3(nDotL), 1.0)) + spec;
}
