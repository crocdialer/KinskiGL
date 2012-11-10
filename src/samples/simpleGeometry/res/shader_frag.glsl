#version 150 core

uniform int u_numTextures;
uniform sampler2D u_textureMap[];

uniform struct
{
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    vec4 emission;

} u_material;

uniform float u_textureMix;
uniform vec3 u_lightDir;

//in float nDotL;
in vec3 v_normal;
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
    
    float nDotL = max(0.0, dot(v_normal, normalize(-u_lightDir)));

    fragData = mix(u_material.diffuse * color1 * vec4(vec3(nDotL), 1.0), jet(nDotL), u_textureMix);
    //u_material.diffuse * mix(color1, vec4(1) - color1 - color2, u_textureMix);
    
    //fragData = u_material.diffuse * mix(color1, vec4(1) - color1 - color2, u_textureMix * nDotL);
}

