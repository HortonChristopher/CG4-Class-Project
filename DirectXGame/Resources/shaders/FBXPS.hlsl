#include "FBX.hlsli"

// PI
static const float PI = 3.141592654f;
// Reflection normal vector
static float3 N;

// Texture set for 0 slot
Texture2D<float4> tex : register(t0);

// Sampler set in 0 slot
SamplerState smp : register(s0);

// Bidiretional Scattering Distribution Function (BSDF)
float3 BRDF(float3 L, float3 V)
{
	// Inner product of normal and light direction
	float NdotL = dot(N, L);
	//Dot product of normal and camera direction
	float NdotV = dot(N, V);
	// Returns black if either is 90 degrees or higher
	if (NdotL < 0 || NdotV < 0) { return float3(0, 0, 0); }

	// Extension reflectance
	float diffuseReflectance = 1.0f / PI;
	// Enlargement
	float3 diffuseColor = diffuseReflectance * NdotL * baseColor * (1 - metalness);

	// Insert the specular formula here
	float intensity = saturate(NdotV);
	float reflection = pow(intensity, 20);
	float3 specularColor = (0,0,0);
	float3 specular = specularColor * reflection;

	//diffuseColor *= intensity;
	diffuseColor += specular;

	return diffuseColor;
}

// Entry point
float4 main(VSOutput input) : SV_TARGET
{
	// Assign the surface information to a static variable so that it can be referenced from the function
	N = input.normal;
	// Final RBG
	float3 finalRGB = float3(0, 0, 0);
	// Direction vector from vertex to viewpoint
	float3 eyedir = normalize(cameraPos - input.worldpos.xyz);

	// Parallel light source
	for (int i = 0; i < DIRLIGHT_NUM; i++)
	{
		if (!dirLights[i].active)
		{
			continue;
		}
		// Combine BRDF results with light colors
		finalRGB += BRDF(dirLights[i].lightv, eyedir) * dirLights[i].lightcolor;
	}

	return float4(finalRGB, 1);
}