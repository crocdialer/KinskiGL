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
    float spotCosCutoff;
    float spotExponent;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
};

vec4 shade(in Lightsource light, in Material mat, in vec3 normal,
           in vec3 eyeVec, in vec3 light_pos, in vec3 light_spot_dir,
           in vec4 base_color, float shade_factor)
{
  vec3 lightDir = light.type > 0 ? (light_pos - eyeVec) : light_spot_dir;
  vec3 L = normalize(lightDir);
  vec3 E = normalize(-eyeVec);
  vec3 R = reflect(-L, normal);
  vec4 ambient = mat.ambient * light.ambient;
  float att = 1.0;
  float nDotL = dot(normal, L);

  if (light.type > 0)
  {
    float dist = length(lightDir);
    att = min(1.f, light.intensity / (light.constantAttenuation +
                   light.linearAttenuation * dist +
                   light.quadraticAttenuation * dist * dist));

    if(light.type > 1)
    {
      float spotEffect = dot(normalize(light_spot_dir), -L);

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
  vec4 diffuse = mat.diffuse * light.diffuse * vec4(att * shade_factor * vec3(nDotL), 1.0);
  vec4 spec = shade_factor * att * mat.specular * light.specular * specIntesity;
  spec.a = 0.0;
  return base_color * (ambient + diffuse) + spec;
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
  vec4 texColors = /*vertex_in.color **/ texture(u_sampler_2D[DIFFUSE],
                                             vertex_in.texCoord.st);

  vec3 normal;
  //normal = normalFromHeightMap(u_sampler_2D[1], vertex_in.texCoord.xy, 0.8);

  normal = normalize(2.0 * (texture(u_sampler_2D[NORMAL],
                                    vertex_in.texCoord.xy).xyz - vec3(0.5)));
  vec4 shade_color = vec4(0, 0, 0, 1);
  int num_lights = min(u_numLights, MAX_NUM_LIGHTS);

  for(int i = 0; i < num_lights; i++)
  {
    shade_color += shade(u_lights[i], u_material, normal, vertex_in.eyeVec,
                         vertex_in.light_position[i],
                         vertex_in.light_direction[i], texColors, 1.0);
  }
  fragData = shade_color;
}
