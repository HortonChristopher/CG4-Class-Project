cbuffer cbuff0 : register(b0)
{
	matrix viewproj; // View projection matrix
	matrix world; // World matrix
	float3 cameraPos; // Camera coordinates (world coordinates)
}

cbuffer cbuff1 : register(b1)
{
	// Albedo
	float3 baseColor;
	// Metalness
	float metalness;
	// Specular Reflection Intensity
	float specular;
	// Roughness
	float roughness;
}

static const int DIRLIGHT_NUM = 3;

struct DirLight
{
	float3 lightv;
	float3 lightcolor;
	uint active;
};

static const int POINTLIGHT_NUM = 3;

struct PointLight
{
	float3 lightpos;
	float3 lightColor;
	float3 lightatten;
	uint active;
};

static const int SPOTLIGHT_NUM = 3;

struct SpotLight
{
	float3 lightv;
	float3 lightpos;
	float3 lightcolor;
	float3 lightatten;
	float2 lightfactoranglecos;
	uint active;
};

cbuffer cbuff2 : register(b2)
{
	float3 ambientLightColor;
	DirLight dirLights[DIRLIGHT_NUM];
	PointLight pointLights[POINTLIGHT_NUM];
	SpotLight spotLights[SPOTLIGHT_NUM];
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