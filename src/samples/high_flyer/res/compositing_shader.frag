#version 150 core
uniform int u_numTextures;

// 0: input
// 1: mask

#define BACKGROUND 0
#define NEOPHYT 1
#define MASK 2

uniform sampler2D u_textureMap[4];
uniform float bg_factor = 1.0;

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
} vertex_in;
          
out vec4 fragData;
void main()
{
    vec2 flip_coords = vec2(vertex_in.texCoord.s, 1 - vertex_in.texCoord.t);
    vec4 texColors = vertex_in.color;
  
    vec4 background_color = bg_factor * texture(u_textureMap[BACKGROUND], flip_coords);
    vec4 neophyt_color = texture(u_textureMap[NEOPHYT], vertex_in.texCoord.st);
    float mask_val = texture(u_textureMap[MASK], vertex_in.texCoord).r;
    
//    texColors *= neophyt_color * mask_val;
    texColors *= mix(background_color, neophyt_color * mask_val, neophyt_color.a * mask_val);

    fragData = u_material.diffuse * texColors;
}
