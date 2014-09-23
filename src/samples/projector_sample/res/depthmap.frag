#version 150 core
uniform int u_numTextures;

uniform sampler2D u_textureMap[4];

uniform struct Material
{
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    vec4 emission;
    float shinyness;
} u_material;

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
//    float z = 0.5 * ProjCoords.z + 0.5;
//    float Depth = texture(gShadowMap, UVCoords).x;
//    if (Depth < (z + 0.00001))
//    return 0.5;
//    else
//    return 1.0;
}

#define DIFFUSE 0
#define MOVIE 1
#define SHADOWMAP 2

void main()
{
    vec4 texColors = vertex_in.color;
    vec3 proj_coords = projected_coords(vertex_in.lightspace_pos);

    texColors *= texture(u_textureMap[DIFFUSE], vertex_in.texCoord);


    float depth = texture(u_textureMap[SHADOWMAP], proj_coords.xy).x;
    bool is_in_shadow = depth < (proj_coords.z - 0.00001);
    
//    depth = 1.0 - (1.0 - depth) * 50.0;

    vec4 movie_tex = texture(u_textureMap[MOVIE], vec2(proj_coords.x, 1 - proj_coords.y));
    
    texColors += is_in_shadow ? .7 * (vec4(vec3(1) - movie_tex.rgb, 1.0)) : movie_tex;

    fragData = u_material.diffuse * texColors; //vec4(vec3(depth), 1.0);
}
