#version 410
uniform int u_numTextures;

uniform sampler2D u_sampler_2D[4];

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
  vec3 spotDirection; 
  float spotCosCutoff; 
  float spotExponent; 
  float constantAttenuation; 
  float linearAttenuation; 
  float quadraticAttenuation; 
};

layout(std140) uniform MaterialBlock
{
  Material u_material;
};

layout(std140) uniform LightBlock
{
  int u_numLights;
  Lightsource u_lights[16];
};

in VertexData{
   vec4 color;
   vec2 texCoord;
   vec4 lightspace_pos;
} vertex_in;
          
out vec4 fragData;

vec3 projected_coords(vec4 the_lightspace_pos)
{
    vec3 proj_coords = the_lightspace_pos.xyz / the_lightspace_pos.w;
    proj_coords = (vec3(1) + proj_coords) * 0.5;
    return proj_coords;
}

#define DIFFUSE 0
#define MOVIE 1
#define SHADOWMAP 2

void main()
{
    vec4 texColors = vertex_in.color;
    vec3 proj_coords = projected_coords(vertex_in.lightspace_pos);

    texColors *= texture(u_sampler_2D[DIFFUSE], vertex_in.texCoord);

    float depth = texture(u_sampler_2D[SHADOWMAP], proj_coords.xy).x;
    bool is_in_shadow = depth < (proj_coords.z - 0.00001);
    
//    depth = 1.0 - (1.0 - depth) * 50.0;

    vec4 movie_tex = texture(u_sampler_2D[MOVIE], vec2(proj_coords.x, 1 - proj_coords.y));
    
    //texColors += is_in_shadow ? .7 * (vec4(vec3(1) - movie_tex.rgb, 1.0)) : movie_tex;
    texColors += is_in_shadow ? vec4(0, 0, 0, 1) : movie_tex;

    fragData = u_material.diffuse * texColors; //vec4(vec3(depth), 1.0);
}
