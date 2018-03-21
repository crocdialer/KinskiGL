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

vec4 shade(in Lightsource light, in Material mat, in vec3 normal, in vec3 eyeVec, in vec4 base_color)
{
  vec3 lightDir = light.type > 0 ? (light.position - eyeVec) : -light.direction;
  vec3 L = normalize(lightDir);
  vec3 E = normalize(-eyeVec);
  vec3 R = reflect(-L, normal);
  vec4 ambient = mat.ambient * light.ambient;
  float att = 1.0;
  float nDotL = dot(normal, L);

  if (light.type > 0)
  {
      // distance^2
      float dist2 = dot(lightDir, lightDir);
      float v = clamp(1.f - pow(dist2 / (light.radius * light.radius), 2.f), 0.f, 1.f);
      att = min(1.f, light.intensity * v * v / (1.f + dist2 * light.quadraticAttenuation));

    if(light.type > 1)
    {
      float spotEffect = dot(normalize(light.direction), -L);
      if (spotEffect < light.spotCosCutoff)
      {
        att = 0.0;
        base_color * ambient;
      }
      spotEffect = pow(spotEffect, light.spotExponent);
      att *= spotEffect;
    }
  }
  nDotL = max(0.0, nDotL);
  float specIntesity = clamp(pow( max(dot(R, E), 0.0), mat.shinyness), 0.0, 1.0);
  vec4 diffuse = att * mat.diffuse * light.diffuse * vec4(vec3(nDotL), 1.0);
  vec4 spec = att * mat.specular * light.specular * specIntesity; spec.a = 0.0;
  return base_color * (ambient + diffuse) + spec;
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
  Lightsource u_lights[MAX_NUM_LIGHTS];
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
      shade_color += shade(u_lights[i], u_material, normal, eyeVec, vec4(1));
  }
  vertex_out.color = a_color * shade_color;
  gl_Position = u_modelViewProjectionMatrix * a_vertex;
}
