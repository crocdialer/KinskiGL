#version 410 core
#extension GL_ARB_separate_shader_objects : enable

#define PI 3.1415926535897932384626433832795
#define ONE_OVER_PI 0.31830988618379067153776752674503


// unfiltered map with mips
uniform samplerCube u_sampler_cube[1];

uniform float u_roughness;

vec2 Hammersley(uint i, uint N)
{
	float vdc = float(bitfieldReverse(i)) * 2.3283064365386963e-10; // Van der Corput
	return vec2(float(i) / float(N), vdc);
}

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

float D_GGX(float roughness, float NoH)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float denom = NoH * NoH * (a2 - 1.0) + 1.0;
	denom = PI * denom * denom;
	return a2 / denom;
}

vec3 ImportanceSample(vec3 R)
{
	// Approximation: assume V == R
	// We lose enlongated reflection by doing this but we also get rid
	// of one variable. So we are able to bake irradiance into a single
	// mipmapped cube map. Otherwise, a cube map array is required.
	vec3 N = R;
	vec3 V = R;
	vec4 result = vec4(0.0);

	float cubeWidth = float(textureSize(u_sampler_cube[0], 0).x);

	const uint numSamples = 1024;
	for (uint i = 0; i < numSamples; ++i)
	{
		vec2 Xi = Hammersley(i, numSamples);
		vec3 H = ImportanceSampleGGX(Xi, u_roughness, N);
		vec3 L = 2.0 * dot(V, H) * H - V;

		float NoL = max(dot(N, L), 0.0);
		float NoH = max(dot(N, H), 0.0);
		float VoH = max(dot(V, H), 0.0);

		if(NoL > 0.0)
		{
			float D = D_GGX(u_roughness, NoH);
			float pdf = D * NoH / (4.0 * VoH);
			float solidAngleTexel = 4 * PI / (6.0 * cubeWidth * cubeWidth);
			float solidAngleSample = 1.0 / (numSamples * pdf);
			float lod = u_roughness == 0.0 ? 0.0 : 0.5 * log2(solidAngleSample / solidAngleTexel);

			vec3 hdrRadiance = textureLod(u_sampler_cube[0], L, lod).rgb;
			result += vec4(hdrRadiance * NoL, NoL);
		}
	}
	if(result.w == 0.0){ return result.rgb; }
	else{ return result.rgb / result.w;	}
}

in VertexData
{
    vec3 eyeVec;
} vertex_in;

out vec4 fragData;

void main()
{
	vec3 R = normalize(vertex_in.eyeVec);

	// Convolve the environment map with a GGX lobe along R
	fragData = vec4(ImportanceSample(R), 1.0);
}
