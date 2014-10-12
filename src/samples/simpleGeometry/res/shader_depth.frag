#version 330

uniform int u_numTextures;
uniform sampler2D u_sampler_2D[1];
uniform float u_near, u_far;

in vec4 v_texCoord;
out vec4 fragData;

float linearizeDepth(float depth, float near, float far)  
{  
	return -far * near / (depth * (far - near) - far);
}

vec4 jet(in float val)
{
    return vec4(min(4.0 * val - 1.5, -4.0 * val + 4.5),
                min(4.0 * val - 0.5, -4.0 * val + 3.5),
                min(4.0 * val + 0.5, -4.0 * val + 2.5),
                1.0);
}

void main()
{
    float depth = texture(u_sampler_2D[0], v_texCoord.st).r;
    depth = linearizeDepth(depth, u_near, u_far);
    fragData = jet(1.0 - depth);
}

