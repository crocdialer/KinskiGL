#version 410

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

// vec4 shade(in Lightsource light, in Material mat, in vec3 normal, in vec3 eyeVec, in vec4 base_color,
//            float shade_factor)
// {
//   vec3 lightDir = light.type > 0 ? (light.position - eyeVec) : light.direction;
//   vec3 L = normalize(lightDir);
//   vec3 E = normalize(-eyeVec);
//   vec3 R = reflect(-L, normal);
//   vec4 ambient = mat.ambient * light.ambient;
//   float att = 1.0;
//   float nDotL = dot(normal, L);
//
//   if (light.type > 0)
//   {
//     float dist = length(lightDir);
//     att = min(1.f, light.intensity / (light.constantAttenuation +
//                    light.linearAttenuation * dist +
//                    light.quadraticAttenuation * dist * dist));
//
//     if(light.type > 1)
//     {
//       float spotEffect = dot(normalize(light.direction), -L);
//
//       if (spotEffect < light.spotCosCutoff)
//       {
//         att = 0.0;
//         base_color * ambient;
//       }
//       spotEffect = pow(spotEffect, light.spotExponent);
//       att *= spotEffect;
//     }
//   }
//   nDotL = max(0.0, nDotL);
//   float specIntesity = clamp(pow( max(dot(R, E), 0.0), mat.shinyness), 0.0, 1.0);
//   vec4 diffuse = mat.diffuse * light.diffuse * vec4(att * shade_factor * vec3(nDotL), 1.0);
//   vec4 spec = shade_factor * att * mat.specular * light.specular * specIntesity;
//   spec.a = 0.0;
//   return base_color * (ambient + diffuse) + spec;
// }

layout(std140) uniform LightBlock
{
  int u_numLights;
  Lightsource u_lights[];
};

// window dimension
uniform vec2 u_window_dimension;

// regular textures
uniform int u_numTextures;
uniform sampler2D u_sampler_2D[4];

#define ALBEDO 0
#define NORMAL 1
#define POSITION 2
#define TEX_COORD 3

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

    vec4 shade_color = color;//vec4(1, 0, 0, 1);

    // shade_color += shade(u_lights[0], u_material, normal, vertex_in.eyeVec, texColors, 1.0);
    fragData = shade_color;
}
