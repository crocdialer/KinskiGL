#version 150 core
uniform mat4 u_modelViewMatrix;
uniform mat4 u_modelViewProjectionMatrix;
uniform mat3 u_normalMatrix;
uniform mat4 u_textureMatrix;
                                
in vec4 a_vertex;
in vec4 a_texCoord;
in vec3 a_normal;

out VertexData{
    vec4 color;
    vec4 texCoord;
    vec3 normal;
    vec3 eyeVec;
    vec3 eyeReflect;
} vertex_out;
          
void main()
{
    vertex_out.normal = normalize(u_normalMatrix * a_normal);
    vertex_out.texCoord = u_textureMatrix * a_texCoord;
    vertex_out.eyeVec = (u_modelViewMatrix * a_vertex).xyz;
    vertex_out.eyeReflect = reflect(vertex_out.eyeVec, vertex_out.normal);
    gl_Position = u_modelViewProjectionMatrix * a_vertex;
}
