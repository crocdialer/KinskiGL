#version 410

in VertexData
{
  vec3 position;
} vertex_in;

out vec4 out_position;

void main()
{
    out_position = vec4(vertex_in.position, 1.0);
}
