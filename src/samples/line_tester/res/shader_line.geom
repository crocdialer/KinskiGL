#version 150

// ------------------ Geometry Shader --------------------------------
layout(lines) in;
layout (triangle_strip, max_vertices = 4) out;

uniform float u_line_thickness;
uniform vec2 u_window_size;

in VertexData {
    vec4 color;
    vec2 texCoord;
} vertex_in[2];

out VertexData {
    vec4 color;
    vec2 texCoord;
} vertex_out;

void main()
{
    // get the four vertices passed to the shader:
    vec2 p0 = gl_in[0].gl_Position.xy;	// start of current segment
    vec2 p1 = gl_in[1].gl_Position.xy;	// end of current segment
    
    // determine the direction of the segment
    vec2 v0 = normalize(p1 - p0);

    // determine the normal
    vec2 n0 = vec2(-v0.y, v0.x);

    // generate the triangle strip
    vec2 bias = n0 * u_line_thickness / u_window_size;
    
    vertex_out.color = vertex_in[0].color;
    vertex_out.texCoord = vec2(0, 1);
    gl_Position = vec4(p0 + bias , 0, 1);
    EmitVertex();
    
    vertex_out.color = vertex_in[0].color;
    vertex_out.texCoord = vec2(0, 0);
    gl_Position = vec4(p0 - bias, 0, 1);
    EmitVertex();
    
    vertex_out.color = vertex_in[1].color;
    vertex_out.texCoord = vec2(0, 1);
    gl_Position = vec4(p1 + bias, 0, 1);
    EmitVertex();
    
    vertex_out.color = vertex_in[1].color;
    vertex_out.texCoord = vec2(0, 0);
    gl_Position = vec4(p1 - bias, 0, 1);
    EmitVertex();
    
    EndPrimitive();
}