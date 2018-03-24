#version 410

#define MAX_NUM_LIGHTS 8

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
    vec3 diffuse = att * vec3(nDotL) * light.diffuse.rgb;
    vec3 specular = att * light.specular.rgb * F_schlick(the_spec.rgb, nDotL) * D_blinn(nDotH, the_spec.a);
    return base_color * vec4(ambient + diffuse, 1.0) + vec4(specular, 0);
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
  vec3 spherePosEye = -(vertex_in.eyeVec + normal * vertex_in.point_size / 2.0);

  vec4 shade_color = vec4(0);

  int num_lights = min(MAX_NUM_LIGHTS, u_numLights);

  for(int i = 0; i < num_lights; ++i)
  {
      shade_color += shade(u_lights[i], normal, spherePosEye, texColors, u_material.specular, 1.0);
  }

  fragData = shade_color;//texColors * (u_material.diffuse * vec4(vec3(nDotL), 1.0)) + spec;
}
