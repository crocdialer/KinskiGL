// #version 410
// #include brdf.glsl

#define MAX_NUM_LIGHTS 512

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
#define MATERIAL_PROPS 4

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
    vec4 mat_prop = texture(u_sampler_2D[MATERIAL_PROPS], tex_coord);
    fragData = shade(u_lights[u_light_index], normal, position, color, mat_prop, 1.0);
}
