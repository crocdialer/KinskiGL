#version 150 core

uniform sampler2D   u_textureMap[];

uniform struct
{
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    vec4 emission;

} u_material;

uniform float u_textureMix;

in vec4 v_texCoord;
out vec4 fragData;

vec4 jet(in float val)
{
    return vec4(min(4.0 * val - 1.5, -4.0 * val + 4.5),
                min(4.0 * val - 0.5, -4.0 * val + 3.5),
                min(4.0 * val + 0.5, -4.0 * val + 2.5),
                1.0);
}

float gray(in vec3 color)
{
    return dot(color, vec3(0.299, 0.587, 0.114));
}

void main()
{
    vec4 color1 = texture(u_textureMap[0], v_texCoord.xy);
    vec4 color2 = texture(u_textureMap[1], v_texCoord.xy);

    fragData = u_material.diffuse * mix(color1, vec4(1) - color1 - color2, u_textureMix);
}

