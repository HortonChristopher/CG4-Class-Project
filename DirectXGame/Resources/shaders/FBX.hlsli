cbuffer cbuff0 : register(b0)
{
	matrix viewproj; // View projection matrix
	matrix world; // World matrix
	float3 cameraPos; // Camera coordinates (world coordinates)
}

// Vertex Buffer Entry
struct VSInput
{
	float4 pos : POSITION; // Position
	float3 normal : NORMAL; // Normal
	float2 uv : TEXCOORD; // Texture Coordinates
};

// Used for communication from the point shader to the pixel shader
struct VSOutput
{
	float4 svpos : SV_POSITION; // Vertex coordinates for system
	float3 normal : NORMAL; // Normal
	float2 uv : TEXCOORD; // UV
};