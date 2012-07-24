#version 150 core

uniform sampler2D   u_textureMap[2];
in vec4             v_texCoord;

out vec4 fragData;

void main()
{
    vec4 color = texture(u_textureMap[0], v_texCoord.xy);
    float confidence = texture(u_textureMap[1], v_texCoord.xy).r;

    vec4 gray = vec4(vec3(dot(color.rgb, vec3(0.299, 0.587, 0.114))), 1.0);

    fragData = mix(gray, color, confidence) ;
}

