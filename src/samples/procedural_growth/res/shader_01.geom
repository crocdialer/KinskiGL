#version 150

// ------------------ Geometry Shader: Line -> Cuboid --------------------------------
layout(lines) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_modelViewMatrix;
uniform mat3 u_normalMatrix;

// placeholder for halfs of depth and height
float depth2 = .5;
float height2 = .5;
vec3 cap_bias = vec3(.1, 0, 0);

// scratch space for our cuboid vertices
vec3 v[8];

// projected cuboid vertices
vec4 vp[8];

vec4 texCoords[4]; 


// cubiod vertices in eye coords
vec3 eye_vecs[8];

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
    
    depth2 = height2 = vertex_in[0].pointSize / 2.0;

    // basevectors for the cuboid
    vec3 dx = normalize(p1 - p0);
    vec3 dy = normalize(vertex_in[0].normal);
    vec3 dz = cross(dx, dy);

    //calc 8 vertices that define our cuboid
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
    
    // calculate projected coords
    for(int i = 0; i < 8; i++){ vp[i] = u_modelViewProjectionMatrix * vec4(v[i], 1); }
    
    // calcualte eye coords
    for(int i = 0; i < 8; i++){ eye_vecs[i] = (u_modelViewMatrix * vec4(v[i], 1)).xyz; }

    // texCoords
    texCoords[0] = vec4(0, 0, 0, 1);
    texCoords[1] = vec4(1, 0, 0, 1);
    texCoords[2] = vec4(0, 1, 0, 1);
    texCoords[3] = vec4(1, 1, 0, 1);

    // generate a triangle strip
    vertex_out.color = vertex_in[0].color;
    //vertex_out.texCoord = vec4(0, 0, 0, 1);
    //vertex_out.eyeVec = (u_modelViewMatrix * vec4(p0, 1)).xyz;
    dx *= u_normalMatrix; 
    dy *= u_normalMatrix; 
    dz *= u_normalMatrix;

    // mantle faces
    
    // front
    vertex_out.normal = dz;
    vertex_out.eyeVec = eye_vecs[0];
    vertex_out.texCoord = texCoords[0];
    gl_Position = vp[0];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[1];
    vertex_out.texCoord = texCoords[1];
    gl_Position = vp[1];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[4];
    vertex_out.texCoord = texCoords[2];
    gl_Position = vp[4];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[5];
    vertex_out.texCoord = texCoords[3];
    gl_Position = vp[5];
    EmitVertex();
    EndPrimitive();

    // top
    vertex_out.normal = dy;
    vertex_out.eyeVec = eye_vecs[4];
    vertex_out.texCoord = texCoords[2];
    gl_Position = vp[4];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[5];
    vertex_out.texCoord = texCoords[3];
    gl_Position = vp[5];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[7];
    vertex_out.texCoord = texCoords[0];
    gl_Position = vp[7];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[6];
    vertex_out.texCoord = texCoords[1];
    gl_Position = vp[6];
    EmitVertex();
    EndPrimitive();

    // back
    vertex_out.normal = -dz;
    vertex_out.eyeVec = eye_vecs[7];
    vertex_out.texCoord = texCoords[0];
    gl_Position = vp[7];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[6];
    vertex_out.texCoord = texCoords[1];
    gl_Position = vp[6];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[3];
    vertex_out.texCoord = texCoords[2];
    gl_Position = vp[3];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[2];
    vertex_out.texCoord = texCoords[3];
    gl_Position = vp[2];
    EmitVertex();
    EndPrimitive();

    // bottom
    vertex_out.normal = -dy;
    vertex_out.eyeVec = eye_vecs[3];
    vertex_out.texCoord = texCoords[2];
    gl_Position = vp[3];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[2];
    vertex_out.texCoord = texCoords[3];
    gl_Position = vp[2];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[0];
    vertex_out.texCoord = texCoords[0];
    gl_Position = vp[0];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[1];
    vertex_out.texCoord = texCoords[1];
    gl_Position = vp[1];
    EmitVertex();
    EndPrimitive();

    // caps faces left
    //vertex_out.normal = -dx;
    //vertex_out.eyeVec = eye_vecs[3];
    //vertex_out.texCoord = texCoords[0];
    //gl_Position = vp[3];
    //EmitVertex();
    //vertex_out.eyeVec = eye_vecs[0];
    //vertex_out.texCoord = texCoords[0];
    //gl_Position = vp[0];
    //EmitVertex();
    //vertex_out.eyeVec = eye_vecs[7];
    //vertex_out.texCoord = texCoords[0];
    //gl_Position = vp[7];
    //EmitVertex();
    //vertex_out.eyeVec = eye_vecs[4];
    //vertex_out.texCoord = texCoords[0];
    //gl_Position = vp[4];
    //EmitVertex();
    //EndPrimitive();

    // caps faces right
    vertex_out.normal = dx;
    vertex_out.eyeVec = eye_vecs[1];
    vertex_out.texCoord = texCoords[0];
    gl_Position = vp[1];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[2];
    vertex_out.texCoord = texCoords[0];
    gl_Position = vp[2];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[5];
    vertex_out.texCoord = texCoords[0];
    gl_Position = vp[5];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[6];
    vertex_out.texCoord = texCoords[0];
    gl_Position = vp[6];
    EmitVertex();
    EndPrimitive();
}
