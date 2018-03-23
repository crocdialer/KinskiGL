#version 410

#define MAX_NUM_LIGHTS 512

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
    // float constantAttenuation;
    // float linearAttenuation;
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

layout(std140) uniform LightBlock
{
  int u_numLights;
  Lightsource u_lights[MAX_NUM_LIGHTS];
};

// window dimension
uniform vec2 u_window_dimension;
uniform int u_light_index;

// regular textures
uniform int u_numTextures;
uniform sampler2D u_sampler_2D[5];

#define ALBEDO 0
#define NORMAL 1
#define POSITION 2
#define EMISSION 3
#define SPECULAR 4

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
    vec4 specular = texture(u_sampler_2D[SPECULAR], tex_coord);
    specular = vec4(specular.r, specular.r, specular.r, specular.g);
    fragData = shade(u_lights[u_light_index], normal, position, color, specular, 1.0);
    //fragData = vec4(1, 0, 0, 1);
}
