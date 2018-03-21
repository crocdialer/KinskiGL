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
    float nDotL = dot(normal, L);

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
    nDotL = max(0.f, nDotL);

    // phong speculars
    // float specIntesity = clamp(pow(max(dot(R, E), 0.0), the_spec.a), 0.0, 1.0);

    // blinn-phong speculars
    float specIntesity = clamp(pow(max(dot(normal, H), 0.0), 4 * the_spec.a), 0.0, 1.0);

    vec4 diffuse = light.diffuse * vec4(att * shade_factor * vec3(nDotL), 1.0);
    vec3 final_spec = shade_factor * att * the_spec.rgb * light.specular.rgb * specIntesity;
    return base_color * (ambient + diffuse) + vec4(final_spec, 0);
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
