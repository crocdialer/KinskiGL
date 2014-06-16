#version 150

// ------------------ Geometry Shader: Line -> Cuboid --------------------------------
layout(lines) in;
//layout (triangles, max_vertices = 36) out;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_modelViewMatrix;
uniform mat3 u_normalMatrix;

// placeholder for halfs of depth and height
float depth2 = .5;
float height2 = .5;

// scratch space for our cuboid vertices
vec4 v[8];

in VertexData 
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec2 texCoord;
    float pointSize;
} vertex_in[2];

out VertexData 
{
  vec4 color;
  vec4 texCoord; 
  vec3 normal; 
  vec3 eyeVec;
} vertex_out;

void main()
{
    vec3 p0 = vertex_in[0].position;	// start of current segment
    vec3 p1 = vertex_in[1].position;	// end of current segment
    
    //float d2_left, d2_right;
    //d2_left = d2_right = vertex_in[0].point_size / 2.0;
    //float h2_left, h2_right;
    //h2_left = h2_right = vertex_in[1].point_size / 2.0;


    // basevectors for the cuboid
    vec3 dx = normalize(p1 - p0);
    vec3 dy = vertex_in[0].normal;
    vec3 dz = cross(dx, dy);

    //TODO: calc 8 vertices that define our cuboid
    
    // front left bottom
    v[0] = u_modelViewProjectionMatrix * vec4(p0 + depth2 * dz - height2 * dy, 1);
    // front right bottom
    v[1] = u_modelViewProjectionMatrix * vec4(p1 + depth2 * dz - height2 * dy, 1);
    // back right bottom
    v[2] = u_modelViewProjectionMatrix * vec4(p1 - depth2 * dz - height2 * dy, 1);
    // back left bottom
    v[3] = u_modelViewProjectionMatrix * vec4(p0 - depth2 * dz - height2 * dy, 1);

    // front left top
    v[4] = u_modelViewProjectionMatrix * vec4(p0 + depth2 * dz + height2 * dy, 1);
    // front right top
    v[5] = u_modelViewProjectionMatrix * vec4(p1 + depth2 * dz + height2 * dy, 1);
    // back right top
    v[6] = u_modelViewProjectionMatrix * vec4(p1 - depth2 * dz + height2 * dy, 1);
    // back left top
    v[7] = u_modelViewProjectionMatrix * vec4(p0 - depth2 * dz + height2 * dy, 1);
    
    // generate a triangle strip
    vertex_out.color = vertex_in[0].color;
    vertex_out.texCoord = vec4(0, 0, 0, 1);
    vertex_out.eyeVec = (u_modelViewMatrix * vec4(p0, 1)).xyz;

    // mantle faces

    // front
    vertex_out.normal = dz;
    gl_Position = v[0];
    EmitVertex();
    gl_Position = v[1];
    EmitVertex();
    gl_Position = v[4];
    EmitVertex();
    gl_Position = v[5];
    EmitVertex();

    // top
    vertex_out.normal = dy;
    gl_Position = v[7];
    EmitVertex();
    gl_Position = v[6];
    EmitVertex();

    // back
    vertex_out.normal = -dz;
    gl_Position = v[3];
    EmitVertex();
    gl_Position = v[2];
    EmitVertex();

    // bottom
    vertex_out.normal = -dy;
    gl_Position = v[0];
    EmitVertex();
    gl_Position = v[1];
    EmitVertex();
    EndPrimitive();

    // caps faces left
    vertex_out.normal = -dx;
    gl_Position = v[3];
    EmitVertex();
    gl_Position = v[0];
    EmitVertex();
    gl_Position = v[7];
    EmitVertex();
    gl_Position = v[4];
    EmitVertex();
    EndPrimitive();

    // caps faces right
    vertex_out.normal = dx;
    gl_Position = v[1];
    EmitVertex();
    gl_Position = v[2];
    EmitVertex();
    gl_Position = v[5];
    EmitVertex();
    gl_Position = v[6];
    EmitVertex();
    EndPrimitive();
}
