#include "FBX.hlsli"

// Texture set for 0 slot
Texture2D<float4> tex : register(t0);

// Sampler set in 0 slot
SamplerState smp : register(s0);

// Entry point
float4 main(VSOutput input) : SV_TARGET
{
	return float4(ambientLightColor, 1);
}