#version 330
uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;

uniform mat4 u_light_mvp;

in vec4 a_vertex;
in vec4 a_texCoord;
in vec4 a_color;

out VertexData{
   vec4 color;
   vec2 texCoord;
   vec4 lightspace_pos;
} vertex_out;

void main()
{
   vertex_out.color = a_color;
   vertex_out.texCoord =  (u_textureMatrix * a_texCoord).xy;
   vertex_out.lightspace_pos = u_light_mvp * a_vertex;
   
   gl_Position = u_modelViewProjectionMatrix * a_vertex;
}
