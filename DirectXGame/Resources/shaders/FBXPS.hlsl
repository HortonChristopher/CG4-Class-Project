#include "FBX.hlsli"

// PI
static const float PI = 3.141592654f;
// Reflection normal vector
static float3 N;

// Texture set for 0 slot
Texture2D<float4> tex : register(t0);

// Sampler set in 0 slot
SamplerState smp : register(s0);

// UE4 Smith Model
float GeometricSmith(float cosine)
{
	float k = (roughness + 1.0f);

	k = k * k / 8.0f;

	return cosine / (cosine * (1.0f - k) + k);
}

float3 SchlickFresnel3(float3 f0, float3 f90, float cosine)
{
	float m = saturate(1 - cosine);
	float m2 = m * m;
	float m5 = m2 * m2 * m;
	return lerp(f0, f90, m5);
}

// Disney Fresnel
float3 DisneyFresnel(float LdotH)
{
	// Fresnel
	// Luminance
	float luminance = 0.3f * baseColor.r + 0.6f * baseColor.g + 0.1f * baseColor.b;

	// Tint
	float3 tintColor = baseColor / luminance;

	// NonMetalColor
	float3 nonMetalColor = specular * 0.08f * tintColor;

	// Metalness Specular Color
	float3 specularColor = lerp(nonMetalColor, baseColor, metalness);

	// NdotH SchlickFresnel
	return SchlickFresnel3(specularColor, float3(1, 1, 1), LdotH);
}

// UE4's GGX
// Alpha = roughness ^ 2
// NdotH = Half vector
float DistributionGGX(float alpha, float NdotH)
{
	float alpha2 = alpha * alpha;
	float t = NdotH * NdotH * (alpha2 - 1.0f) + 1.0f;
	return alpha2 / (PI * t * t);
}

// Calculation of follar reflex
float3 CookTorranceSpecular(float NdotL, float NdotV, float NdotH, float LdotH)
{
	// Distribution
	float Ds = DistributionGGX(roughness * roughness, NdotH);

	// Fresnel
	//float3 Fs = float3(1, 1, 1);
	float3 Fs = DisneyFresnel(LdotH);

	// Geometry Attenuation
	//float Gs = 0.1f;
	//float Gs = 1.0f;
	float Gs = GeometricSmith(NdotL) * GeometricSmith(NdotL);

	// Denominator
	float m = 4.0f * NdotL * NdotV;

	// Combine to get the color of surface reflection
	return Ds * Fs * Gs / m;
}

// Schlick near
// f0 and f90 5 lerp
// f0 : Reflectance when light is incident vertically
// f90 : Relectance when light is incident in parallel
// cosine : cosine of angle between two vectors (internal product value)
float SchlickFresnel(float f0, float f90, float cosine)
{
	float m = saturate(1 - cosine);
	float m2 = m * m;
	float m5 = m2 * m2 * m;
	return lerp(f0, f90, m5);
}

// Bidiretional Scattering Distribution Function (BSDF)
float3 BRDF(float3 L, float3 V)
{
	// Inner product of normal and light direction
	float NdotL = dot(N, L);
	//Dot product of normal and camera direction
	float NdotV = dot(N, V);
	// Returns black if either is 90 degrees or higher
	if (NdotL < 0 || NdotV < 0) { return float3(0, 0, 0); }

	// Midway between light direction and camera direction (half vector)
	float3 H = normalize(L + V);

	// Dot product of normal and half vector
	float NdotH = dot(N, H);

	// Dot product of light and half vector
	float LdotH = dot(L, H);

	// Extension reflectance
	float diffuseReflectance = 1.0f / PI;

	float energyBias = 0.5f * roughness;

	// Diffuse reflectance when angle is 90 degrees
	float Fd90 = energyBias + 2 * LdotH * LdotH * roughness;

	// Diffuse reflectance when entering
	float FL = SchlickFresnel(1.0f, Fd90, NdotL);

	// Diffuse reflectance when exiting
	float FV = SchlickFresnel(1.0f, Fd90, NdotV);

	float energyFactor = lerp(1.0f, 1.0f / 1.51f, roughness);

	// Diffuse reflectance until entering and exiting
	float Fd = FL * FV * energyFactor;

	// Enlargement
	//float3 diffuseColor = diffuseReflectance * NdotL * baseColor * (1 - metalness);
	float3 diffuseColor = diffuseReflectance * Fd * baseColor * (1 - metalness); // Schlick

	// Specular Color
	float3 specularColor = CookTorranceSpecular(NdotL, NdotV, NdotH, LdotH);

	//// Insert the specular formula here
	//float intensity = saturate(NdotV);
	//float reflection = pow(intensity, 20);
	//float3 specularColor = (0,0,0);
	//float3 specular = specularColor * reflection;

	////diffuseColor *= intensity;
	//diffuseColor += specular;

	return diffuseColor + specularColor;
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