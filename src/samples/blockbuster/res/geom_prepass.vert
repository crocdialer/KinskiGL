#version 410
uniform mat4 u_modelViewProjectionMatrix;
uniform mat3 u_normalMatrix;
uniform mat4 u_textureMatrix;

in vec4 a_vertex;
in vec3 a_normal;
in vec4 a_color;
in vec2 a_texCoord;
in float a_pointSize;

out VertexData 
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec2 texCoord;
    float pointSize;
} vertex_out;

void main()
{
    vertex_out.position = a_vertex.xyz;
    vertex_out.normal = a_normal;//normalize(u_normalMatrix * a_normal);
    vertex_out.pointSize = a_pointSize;
    vertex_out.color = a_color;
    vertex_out.texCoord =  (u_textureMatrix * vec4(a_texCoord, 0, 1)).xy;
//    gl_Position = u_modelViewProjectionMatrix * a_vertex;
}
