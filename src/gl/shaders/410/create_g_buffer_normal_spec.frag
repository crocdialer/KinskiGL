#version 410

uniform int u_numTextures;
uniform sampler2D u_sampler_2D[3];

#define COLOR 0
#define NORMALMAP 1
#define SPECULARMAP 2

struct Material
{
    vec4 diffuse;
    vec4 ambient;
    vec4 emission;
    vec4 point_vals;// (size, constant_att, linear_att, quad_att)
    float metalness;
    float roughness;
    int shadow_properties;
    int texture_properties;
};

layout(std140) uniform MaterialBlock
{
  Material u_material;
};

in VertexData
{
    // vec4 color;
    vec4 texCoord;
    vec3 normal;
    vec3 eyeVec;
    vec3 tangent;
} vertex_in;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_position;
layout(location = 3) out vec4 out_emission;
layout(location = 4) out vec4 out_material_props;

void main()
{
  // vec4 texColors = vertex_in.color;
  vec4 texColors = texture(u_sampler_2D[COLOR], vertex_in.texCoord.st);
  if(smoothstep(0.0, 1.0, texColors.a) < 0.01){ discard; }
  vec3 normal = normalize(2.0 * (texture(u_sampler_2D[NORMALMAP],
                                 vertex_in.texCoord.xy).xyz - vec3(0.5)));
  mat3 transpose_tbn = mat3(vertex_in.tangent, cross(vertex_in.normal, vertex_in.tangent), vertex_in.normal);
  normal = transpose_tbn * normal;
  out_color = u_material.diffuse * texColors;
  out_normal = vec4(normal, 1);
  out_position = vec4(vertex_in.eyeVec, 1);
  out_emission = u_material.emission * texColors;
  float roughness = 1 - texture(u_sampler_2D[SPECULARMAP], vertex_in.texCoord.xy).x;
  out_material_props = vec4(u_material.metalness, roughness, u_material.shadow_properties & 2, 1);
}
