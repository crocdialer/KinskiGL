#version 410 core
#extension GL_ARB_separate_shader_objects : enable

#define MAX_NUM_LIGHTS 8

struct matrix_struct_t
{
    mat4 model_view;
    mat4 model_view_projection;
    mat4 texture_matrix;
    mat3 normal_matrix;
};

layout(std140) uniform MatrixBlock
{
    matrix_struct_t ubo;
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

layout(std140) uniform LightBlock
{
  int u_numLights;
  Lightsource u_lights[MAX_NUM_LIGHTS];
};

layout(location = 0) in vec4 a_vertex;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_texCoord;
layout(location = 3) in vec4 a_color;
layout(location = 5) in vec3 a_tangent;

out VertexData
{
  vec4 color;
  vec4 texCoord;
  vec3 eyeVec;
  vec3 light_position[MAX_NUM_LIGHTS];
  vec3 light_direction[MAX_NUM_LIGHTS];
} vertex_out;

void main()
{
  vertex_out.color = a_color;
  vertex_out.texCoord = ubo.texture_matrix * a_texCoord;
  vec3 n = normalize(ubo.normal_matrix * a_normal);
  vec3 t = normalize (ubo.normal_matrix * a_tangent);
  vec3 b = cross(n, t);
  mat3 tbnMatrix = transpose(mat3(t, b, n));
  vec3 eye = (ubo.model_view * a_vertex).xyz;
  vertex_out.eyeVec = tbnMatrix * eye;

  for(int i = 0; i < u_numLights; i++)
  {
    vertex_out.light_position[i] = tbnMatrix * u_lights[i].position;
    vertex_out.light_direction[i] = tbnMatrix * u_lights[i].direction;
  }

  gl_Position = ubo.model_view_projection * a_vertex;
}
