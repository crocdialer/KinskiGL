#version 330

struct Material
{
  vec4 diffuse;
  vec4 ambient;
  vec4 specular;
  vec4 emission;
  vec4 point_vals;// (size, constant_att, linear_att, quad_att)
  float shinyness;
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
    float spotCosCutoff;
    float spotExponent;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
    float pad_0, pad_1, pad_2;
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

uniform mat4 u_modelViewMatrix;
uniform mat4 u_modelViewProjectionMatrix;
uniform mat3 u_normalMatrix;
uniform mat4 u_textureMatrix;

layout(std140) uniform MaterialBlock
{
  Material u_material;
};

layout(std140) uniform LightBlock
{
  int u_numLights;
  Lightsource u_lights[];
};

layout(location = 0) in vec4 a_vertex;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_texCoord;
layout(location = 3) in vec4 a_color;

out VertexData
{
  vec4 color;
  vec4 texCoord;
} vertex_out;

void main()
{
  vertex_out.texCoord = u_textureMatrix * a_texCoord;
  vec3 normal = normalize(u_normalMatrix * a_normal);
  vec3 eyeVec = (u_modelViewMatrix * a_vertex).xyz;
  vec4 shade_color = vec4(0);

  int num_lights = min(MAX_NUM_LIGHTS, u_numLights);

  for(int i = 0; i < num_lights; ++i)
  {
      shade_color += shade(u_lights[i], normal, eyeVec, vec4(1), u_material.specular, 1.0);
  }

  vertex_out.color = a_color * shade_color;
  gl_Position = u_modelViewProjectionMatrix * a_vertex;
}
