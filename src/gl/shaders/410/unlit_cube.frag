#version 410 core
#extension GL_ARB_separate_shader_objects : enable

uniform samplerCube u_sampler_cube[1];
uniform float u_gamma = 1.0;

in VertexData
{
    vec3 eyeVec;
} vertex_in;

out vec4 fragData;

void main()
{
    fragData = texture(u_sampler_cube[0], vertex_in.eyeVec);
    if(u_gamma != 1.0){ fragData.rgb = pow(fragData.rgb, vec3(1.0 / u_gamma)); }
}
