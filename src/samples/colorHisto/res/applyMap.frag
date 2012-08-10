#version 150 core

uniform sampler2D   u_textureMap[];
in vec4             v_texCoord;

out vec4 fragData;

vec4 jet(in float val)
{
    return vec4(min(4.0 * val - 1.5, -4.0 * val + 4.5),
                min(4.0 * val - 0.5, -4.0 * val + 3.5),
                min(4.0 * val + 0.5, -4.0 * val + 2.5),
                1.0);
}

vec3 gray(in vec3 color)
{
    return vec3(dot(color, vec3(0.299, 0.587, 0.114)));
}

void main()
{
    vec4 color = texture(u_textureMap[0], v_texCoord.xy);
    float confidence = texture(u_textureMap[2], v_texCoord.xy).r;
    
    vec4 gray = vec4(gray(color.rgb), 1.0);

    fragData = mix(gray, color, confidence) ;
}

