#include "FBX.hlsli"

// Texture set for 0 slot
Texture2D<float4> tex : register(t0);

// Sampler set in 0 slot
SamplerState smp : register(s0);

// Entry point
float4 main(VSOutput input) : SV_TARGET
{
	// Texture mapping
	float4 texcolor = tex.Sample(smp, input.uv);
	// Lambert reflection
	float3 light = normalize(float3(1, -1, 1)); // light facing back to lower right
	float diffuse = saturate(dot(-light, input.normal));
	float brightness = diffuse + 0.3f;
	float4 shadecolor = float4(brightness, brightness, brightness, 1.0f);
	// Combine the color of the shader color and texture
	return shadecolor * texcolor;
}