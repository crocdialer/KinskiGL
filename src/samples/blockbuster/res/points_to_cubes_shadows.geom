#version 410

// ------------------ Geometry Shader: Point -> Cuboid --------------------------------
layout(points) in;
layout (triangle_strip, max_vertices = 24) out;

uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_modelViewMatrix;
uniform mat3 u_normalMatrix;
uniform mat4 u_shadow_matrices[4];

uniform float u_length = 10.0;
//uniform float u_width = 1.0;

// placeholder for halfs of depth and height
float depth2 = .5;
float height2 = .5;

// scratch space for our cuboid vertices
vec3 v[8];

// projected cuboid vertices
vec4 vp[8];

vec4 texCoords[4];

// cuboid vertices in eye coords
vec3 eye_vecs[8];

// cuboid vertices in lightspace
#define NUM_SHADOW_LIGHTS 2
vec4 lightspace_vecs[NUM_SHADOW_LIGHTS * 8];

// cuboid normals
vec3 normals[8]; 

in VertexData 
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec2 texCoord;
    float pointSize;
} vertex_in[1];

out VertexData
{
  vec4 color; 
  vec4 texCoord; 
  vec3 normal; 
  vec3 eyeVec;
  vec4 lightspace_pos[NUM_SHADOW_LIGHTS];
} vertex_out;

void create_box(in vec3 p0, in vec3 p1, in vec3 up_vec)
{
    // same for all verts
    depth2 = height2 = vertex_in[0].pointSize / 2.0;
    vertex_out.color = vertex_in[0].color;

    // basevectors for the cuboid
    vec3 dx = normalize(p1 - p0);
    vec3 dy = normalize(up_vec);
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
    
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++)
    {
      for(int j = 0; j < 8; j++)
      {
        lightspace_vecs[i * 8 + j] = u_shadow_matrices[i] * vec4(v[j], 1);
      }
    }

    // generate a triangle strip
    
    // transform directions 
    dx = u_normalMatrix * dx; 
    dy = u_normalMatrix * dy; 
    dz = u_normalMatrix * dz;

    ///////// mantle faces //////////
    
    // front
    vertex_out.normal = dz;
    vertex_out.eyeVec = eye_vecs[0];
    vertex_out.texCoord = texCoords[0];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++)
    { vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 0]; }
    gl_Position = vp[0];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[1];
    vertex_out.texCoord = texCoords[1];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++)
    { vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 1]; }
    gl_Position = vp[1];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[4];
    vertex_out.texCoord = texCoords[2];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 4]; }
    gl_Position = vp[4];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[5];
    vertex_out.texCoord = texCoords[3];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 5]; }
    gl_Position = vp[5];
    EmitVertex();
    EndPrimitive();

    // top
    vertex_out.normal = dy;
    vertex_out.eyeVec = eye_vecs[4];
    vertex_out.texCoord = texCoords[2];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 4]; }
    gl_Position = vp[4];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[5];
    vertex_out.texCoord = texCoords[3];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 5]; }
    gl_Position = vp[5];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[7];
    vertex_out.texCoord = texCoords[0];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 7]; }
    gl_Position = vp[7];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[6];
    vertex_out.texCoord = texCoords[1];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 6]; }
    gl_Position = vp[6];
    EmitVertex();
    EndPrimitive();

    // back
    vertex_out.normal = -dz;
    vertex_out.eyeVec = eye_vecs[7];
    vertex_out.texCoord = texCoords[0];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 7]; }
    gl_Position = vp[7];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[6];
    vertex_out.texCoord = texCoords[1];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 6]; }
    gl_Position = vp[6];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[3];
    vertex_out.texCoord = texCoords[2];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 3]; }
    gl_Position = vp[3];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[2];
    vertex_out.texCoord = texCoords[3];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 2]; }
    gl_Position = vp[2];
    EmitVertex();
    EndPrimitive();
    
    // bottom
    vertex_out.normal = -dy;
    vertex_out.eyeVec = eye_vecs[3];
    vertex_out.texCoord = texCoords[2];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 3]; }
    gl_Position = vp[3];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[2];
    vertex_out.texCoord = texCoords[3];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 2]; }
    gl_Position = vp[2];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[0];
    vertex_out.texCoord = texCoords[0];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 0]; }
    gl_Position = vp[0];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[1];
    vertex_out.texCoord = texCoords[1];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 1]; }
    gl_Position = vp[1];
    EmitVertex();
    EndPrimitive();
    
    // caps faces left
    vertex_out.normal = -dx;
    vertex_out.eyeVec = eye_vecs[3];
    vertex_out.texCoord = texCoords[0];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 3]; }
    gl_Position = vp[3];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[0];
    vertex_out.texCoord = texCoords[0];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 0]; }
    gl_Position = vp[0];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[7];
    vertex_out.texCoord = texCoords[0];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 7]; }
    gl_Position = vp[7];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[4];
    vertex_out.texCoord = texCoords[0];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 4]; }
    gl_Position = vp[4];
    EmitVertex();
    EndPrimitive();

    // caps faces right
    vertex_out.normal = dx;
    vertex_out.eyeVec = eye_vecs[1];
    vertex_out.texCoord = texCoords[0];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 1]; }
    gl_Position = vp[1];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[2];
    vertex_out.texCoord = texCoords[1];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 2]; }
    gl_Position = vp[2];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[5];
    vertex_out.texCoord = texCoords[2];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 5]; }
    gl_Position = vp[5];
    EmitVertex();
    vertex_out.eyeVec = eye_vecs[6];
    vertex_out.texCoord = texCoords[3];
    for(int i = 0; i < NUM_SHADOW_LIGHTS; i++){ vertex_out.lightspace_pos[i] = lightspace_vecs[i * 8 + 6]; }
    gl_Position = vp[6];
    EmitVertex();
    EndPrimitive();
}

void main()
{
    vec3 p0 = vertex_in[0].position;	// start of current segment
    vec3 p1 = p0 + vertex_in[0].normal * u_length;	// end of current segment
    
    // texCoords
    texCoords[0] = vec4(0, 0, 0, 1);
    texCoords[1] = vec4(1, 0, 0, 1);
    texCoords[2] = vec4(0, 1, 0, 1);
    texCoords[3] = vec4(1, 1, 0, 1);
    
    create_box(p0, p1, vec3(0, 1, 0));
}
