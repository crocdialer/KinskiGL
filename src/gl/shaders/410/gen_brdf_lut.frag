#version 410 core
#extension GL_ARB_separate_shader_objects : enable

#define PI 3.1415926535897932384626433832795
#define ONE_OVER_PI 0.31830988618379067153776752674503

vec2 Hammersley(uint i, uint N)
{
	float vdc = float(bitfieldReverse(i)) * 2.3283064365386963e-10; // Van der Corput
	return vec2(float(i) / float(N), vdc);
}

float G1(float k, float NoV)
{
	return NoV / (NoV * (1.0 - k) + k);
}

float G_Smith(float roughness, float NoV, float NoL)
{
	float alpha = roughness * roughness;
	float k = alpha * 0.5; // use k = (roughness + 1)^2 / 8 for analytic lights
	return G1(k, NoL) * G1(k, NoV);
}

// Sample a half-vector in world space
vec3 ImportanceSampleGGX(vec2 Xi, float roughness, vec3 N)
{
	float a = roughness * roughness;

	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt(clamp((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y), 0.0, 1.0));
	float sinTheta = sqrt(clamp(1.0 - cosTheta * cosTheta, 0.0, 1.0));

	vec3 H = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

	vec3 up = abs(N.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
	vec3 tangent = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);

	return tangent * H.x + bitangent * H.y + N * H.z;
}

vec2 IntegrateBRDF(float roughness, float NoV)
{
	vec3 N = vec3(0.0, 0.0, 1.0);
	vec3 V = vec3(sqrt(clamp(1.0 - NoV * NoV, 0.0, 1.0)), 0.0, NoV); // assuming isotropic BRDF

	float A = 0.0;
	float B = 0.0;

	const uint numSamples = 1024;
	for (uint i = 0; i < numSamples; ++i)
	{
		vec2 Xi = Hammersley(i, numSamples);
		vec3 H = ImportanceSampleGGX(Xi, roughness, N);
		vec3 L = 2.0 * dot(V, H) * H - V;

		float NoL = clamp(L.z, 0.0, 1.0);
		float NoH = clamp(H.z, 0.0, 1.0);
		float VoH = clamp(dot(V, H), 0.0, 1.0);

		if (NoL > 0.0)
		{
			float G = G_Smith(roughness, NoV, NoL);

			float G_Vis = G * VoH / (NoH * NoV);
			float Fc = pow(1.0 - VoH, 5.0);

			A += (1.0 - Fc) * G_Vis;
			B += Fc * G_Vis;
		}
	}

	return vec2(A, B) / float(numSamples);
}

uniform vec2 u_window_dimension;

in VertexData
{
  vec4 color;
  vec2 texCoord;
} vertex_in;

out vec4 fragData;

void main()
{
    vec2 tex_coord = gl_FragCoord.xy / u_window_dimension;
    float roughness = tex_coord.y;
	float NoV = tex_coord.x;
	fragData = vec4(IntegrateBRDF(roughness, NoV), 0.0, 1.0);
}
