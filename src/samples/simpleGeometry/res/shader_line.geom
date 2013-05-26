#version 150

layout(lines) in;
layout (triangle_strip, max_vertices = 4) out;

uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;
uniform float u_line_thickness;

in VertexData {
    vec4 color;
    vec4 texCoord;
} vertex_in[2];

out VertexData {
    vec4 color;
    vec4 texCoord;
} vertex_out;

void main()
{
    vec3 line_dir = normalize(gl_in[1].gl_Position - gl_in[0].gl_Position).xyz;
    vec3 offset = vec3(line_dir.y, -line_dir.x, 0.f) / 2.f;
    
    for(int i = 0; i < gl_in.length(); i++)
    {
        // copy attributes
        gl_Position = gl_in[i].gl_Position - vec4(u_line_thickness * offset, 0.f);;
        vertex_out.color = vertex_in[i].color;
        vertex_out.texCoord = vertex_in[i].texCoord;
        
        // done with the vertex
        EmitVertex();
        
        // copy attributes
        gl_Position = gl_in[i].gl_Position + vec4(u_line_thickness * offset, 0.f);
        vertex_out.color = vertex_in[i].color;
        vertex_out.texCoord = vertex_in[i].texCoord;
        
        // done with the vertex
        EmitVertex();
    }
}