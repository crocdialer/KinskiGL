#version 150 core
uniform int u_numTextures;

uniform sampler2D u_textureMap[4];
uniform sampler2DArray u_array_sampler[2];
//uniform sampler3D u_array_sampler[2];

uniform float u_array_index = 0.0;
uniform float u_texture_mix;
uniform vec2 u_mouse_pos;

uniform int u_num_frames = 165;

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
    vec4 texColors = vertex_in.color;
    
    float noise_val = texture(u_textureMap[0], vertex_in.texCoord).r;
    
    vec3 tex_coord = vec3(vertex_in.texCoord, noise_val * u_num_frames);

    vec4 video_color = texture(u_array_sampler[0], tex_coord);

    fragData = u_material.diffuse * texColors * video_color;
}
