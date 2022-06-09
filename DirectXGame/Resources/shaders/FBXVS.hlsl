#include "FBX.hlsli"

// Entry point
VSOutput main(VSInput input)
{
	// Apply scaling and rotation by world matrix to normals
	float4 wnormal = normalize(mul(world, float4(input.normal, 0)));
	// Value to pass to the pixel shader
	VSOutput output;
	// Coordinate change due to matrix
	output.svpos = mul(mul(viewproj, world), input.pos);
	// Pass the world normal to the final stage
	output.normal = wnormal.xyz;
	// Pass the input value as it is to the next stage
	output.uv = input.uv;

	return output;
}