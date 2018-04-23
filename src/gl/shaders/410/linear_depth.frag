#version 410 core

uniform vec2 u_clip_planes;

in VertexData
{
  vec3 eyeVec;
} vertex_in;

void main()
{
    gl_FragDepth = (length(vertex_in.eyeVec) - u_clip_planes.x) / (u_clip_planes.y - u_clip_planes.x);
}
