#include "FBX.hlsli"

// Enter vertices and normals after skinning
struct SkinOutput
{
	float4 pos;
	float3 normal;
};

SkinOutput ComputeSkin(VSInput input)
{
	// Clear Zero
	SkinOutput output = (SkinOutput)0;

	uint iBone; // Bone number to calculate
	float weight; // Bone weight
	matrix m; // Skinning matrix

	// Bone 0
	iBone = input.boneIndices.x;
	weight = input.boneWeights.x;
	m = matSkinning[iBone];
	output.pos += weight * mul(m, input.pos);
	output.normal += weight * mul((float3x3)m, input.normal);

	// Bone 1
	iBone = input.boneIndices.y;
	weight = input.boneWeights.y;
	m = matSkinning[iBone];
	output.pos += weight * mul(m, input.pos);
	output.normal += weight * mul((float3x3)m, input.normal);

	// Bone 2
	iBone = input.boneIndices.z;
	weight = input.boneWeights.z;
	m = matSkinning[iBone];
	output.pos += weight * mul(m, input.pos);
	output.normal += weight * mul((float3x3)m, input.normal);

	// Bone 3
	iBone = input.boneIndices.w;
	weight = input.boneWeights.w;
	m = matSkinning[iBone];
	output.pos += weight * mul(m, input.pos);
	output.normal += weight * mul((float3x3)m, input.normal);

	return output;
}

// Skinning Calculation
//SkinOutput ComputeSkin(VSInput input)
//{
//	// Clear zero
//	SkinOutput output;
//
//	uint iBone; // Bone number to calculate
//	matrix m; // Skinning line
//
//	// Bone 0 only
//	iBone = input.boneIndices.x;
//	m = matSkinning[iBone];
//	output.pos = mul(m, input.pos);
//	output.normal = mul((float3x3)m, input.normal);
//
//	return output;
//}

// Entry point
VSOutput main(VSInput input)
{
	// Skinning calculation
	SkinOutput skinned = ComputeSkin(input);
	// Apply scaling and rotation by world matrix to normals
	float4 wnormal = normalize(mul(world, float4(input.normal, 0)));
	// Value to pass to the pixel shader
	VSOutput output;
	// Coordinate change due to matrix
	output.svpos = mul(mul(viewproj, world), skinned.pos);
	// Pass the world normal to the final stage
	output.normal = wnormal.xyz;
	// Pass the input value as it is to the next stage
	output.uv = input.uv;

	return output;
}