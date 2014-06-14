#version 150

// ------------------ Geometry Shader: Line -> Cuboid --------------------------------
layout(lines) in;
//layout (triangles, max_vertices = 36) out;
layout (triangle_strip, max_vertices = 3) out;

uniform mat4 u_modelViewProjectionMatrix;
uniform mat3 u_normalMatrix;

// placeholder for halfs of depth and height
float depth2 = .2;
float height2 = .6;

// scratch space for out cuboid vertices
vec3 v[8];

in VertexData {
    vec3 position;
    vec3 normal;
    vec4 color;
    vec2 texCoord;
    float pointSize;
} vertex_in[2];

out VertexData {
    vec3 normal;
    vec4 color;
    vec2 texCoord;
} vertex_out;

void main()
{
    vec3 p0 = vertex_in[0].position;	// start of current segment
    vec3 p1 = vertex_in[1].position;	// end of current segment

    // basevectors for the cuboid
    vec3 dx = normalize(p1 - p0);
    vec3 dy = vertex_in[0].normal;
    vec3 dz = cross(dx, dy);

    //TODO: calc 8 vertices that define our cuboid
    
    // front left bottom
    v[0] = p0 + depth2 * dz - height2 * dy;
    // front right bottom
    v[1] = p1 + depth2 * dz - height2 * dy;
    // back right bottom
    v[2] = p1 - depth2 * dz - height2 * dy;
    // back left bottom
    v[3] = p0 - depth2 * dz - height2 * dy;

    // front left top
    v[4] = p0 + depth2 * dz + height2 * dy;
    // front right top
    v[5] = p1 + depth2 * dz + height2 * dy;
    // back right top
    v[6] = p1 - depth2 * dz + height2 * dy;
    // back left top
    v[7] = p0 - depth2 * dz + height2 * dy;
    
    //TODO: generate 12 triangles, 2 for each side

    // generate a triangle
    vertex_out.color = vertex_in[0].color;
    vertex_out.texCoord = vec2(0, 0);
    gl_Position = u_modelViewProjectionMatrix * vec4(p0, 1);
    EmitVertex();
    
    vertex_out.color = vertex_in[0].color;
    vertex_out.texCoord = vec2(.5, 1);
    gl_Position = u_modelViewProjectionMatrix * vec4(height2 * dy + p0 + (p1 - p0) / 2.0, 1);
    EmitVertex();
    
    vertex_out.color = vertex_in[1].color;
    vertex_out.texCoord = vec2(0, 1);
    gl_Position = u_modelViewProjectionMatrix * vec4(p1, 1);
    EmitVertex();

    EndPrimitive();


    //// get the four vertices passed to the shader:
    //vec2 p0 = gl_in[0].gl_Position.xy;	// start of current segment
    //vec2 p1 = gl_in[1].gl_Position.xy;	// end of current segment
    //
    //// determine the direction of the segment
    //vec2 v0 = normalize(p1 - p0);

    //// determine the normal
    //vec2 n0 = vec2(-v0.y, v0.x);

    //// generate the triangle strip
    //vec2 bias = n0 * u_line_thickness / u_window_size;
    //
    //vertex_out.color = vertex_in[0].color;
    //vertex_out.texCoord = vec2(0, 1);
    //gl_Position = vec4(p0 + bias , 0, 1);
    //EmitVertex();
    //
    //vertex_out.color = vertex_in[0].color;
    //vertex_out.texCoord = vec2(0, 0);
    //gl_Position = vec4(p0 - bias, 0, 1);
    //EmitVertex();
    //
    //vertex_out.color = vertex_in[1].color;
    //vertex_out.texCoord = vec2(0, 1);
    //gl_Position = vec4(p1 + bias, 0, 1);
    //EmitVertex();
    //
    //vertex_out.color = vertex_in[1].color;
    //vertex_out.texCoord = vec2(0, 0);
    //gl_Position = vec4(p1 - bias, 0, 1);
    //EmitVertex();
    //
    //EndPrimitive();
}
