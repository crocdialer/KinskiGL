#version 150 core
uniform int u_numTextures;

uniform sampler2D u_sampler_2D[2];
//uniform sampler2DArray u_array_sampler[2];
uniform sampler3D u_sampler_3D[2];

uniform int u_current_index = 0;
uniform int u_num_frames = 165;

struct Material
{
  vec4 diffuse; 
  vec4 ambient; 
  vec4 specular; 
  vec4 emission; 
  vec4 point_vals;// (size, constant_att, linear_att, quad_att) 
  float shinyness;
};

layout(std140) uniform MaterialBlock
{
  Material u_material;
};

in VertexData{
   vec4 color;
   vec2 texCoord;
} vertex_in;
          
out vec4 fragData;
void main()
{
    vec4 texColors = vertex_in.color;
    
    // sample our simplex texture
    float noise_val = texture(u_sampler_2D[0], vertex_in.texCoord).r;
    
    // the time coordinate
    float index = (int(u_current_index + noise_val * u_num_frames) % u_num_frames) / float(u_num_frames);
    
    // 3d texture coordinate
    vec3 tex_coord = vec3(vertex_in.texCoord, index);
    
    // sample our samplerArray / 3DTexture
    vec4 video_color = texture(u_sampler_3D[0], tex_coord);

    fragData = u_material.diffuse * texColors * video_color;
}
