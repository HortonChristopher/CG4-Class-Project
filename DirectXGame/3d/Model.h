#pragma once

#include <string>
#include <vector>
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <fbxsdk.h>

struct Node
{
	// Name
	std::string name;
	// Local Scale
	DirectX::XMVECTOR scaling = { 1,1,1,0 };
	// Local Rotation
	DirectX::XMVECTOR rotation = { 0,0,0,0 };
	// Local Move/Translation
	DirectX::XMVECTOR translation = { 0,0,0,1 };
	// Local transformation matrix
	DirectX::XMMATRIX transform;
	// Global transformation matrix
	DirectX::XMMATRIX globalTransform;
	// Parent Node
	Node* parent = nullptr;
};

class Model
{
private: // Alias
	// Using Microsoft::WRL
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

	// Using DirectX::
	using XMFLOAT2 = DirectX::XMFLOAT2;
	using XMFLOAT3 = DirectX::XMFLOAT3;
	using XMFLOAT4 = DirectX::XMFLOAT4;
	using XMMATRIX = DirectX::XMMATRIX;
	using TexMetadata = DirectX::TexMetadata;
	using ScratchImage = DirectX::ScratchImage;

	// Using std::
	using string = std::string;
	template <class T> using vector = std::vector<T>;


public: // Constant
	// Maximum number of bone instances
	static const int MAX_BONE_INDICES = 4;

public: // Subclass
	// Vertex data structure
	struct VertexPosNormalUvSkin
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT3 uv;
		UINT boneIndex[MAX_BONE_INDICES]; // Bone Number
		float boneWeight[MAX_BONE_INDICES]; // Bone Weight
	};

	// Bone structure
	struct Bone
	{
		// Name
		std::string name;

		// Inverse matrix of initial posture
		DirectX::XMMATRIX invInitialPose;

		// Cluster (FBX example channel information)
		FbxCluster* fbxCluster;

		// Constructor
		Bone(const std::string& name)
		{
			this->name = name;
		}
	};

public:
	// Friend Class
	friend class FbxLoader;

public:
	// Destructor
	~Model();

	// Create Buffer
	void CreateBuffers(ID3D12Device* device);

	// Drawing
	void Draw(ID3D12GraphicsCommandList* cmdList);

	// Get model transformation matrix
	const XMMATRIX& GetModelTransform() { return meshNode->globalTransform; }

	// getter
	FbxScene* GetFbxScene() { return fbxScene; }

	// getter
	std::vector<Bone>& GetBones() { return bones; }

private:
	FbxScene* fbxScene = nullptr;

	// Model Name
	std::string name;

	// Node Array
	std::vector<Node> nodes;

	// Node with mesh
	Node* meshNode = nullptr;

	// Bone Vector
	std::vector<Bone> bones;

	// Vertex data array
	std::vector<VertexPosNormalUvSkin> vertices;

	// Vertex index array
	std::vector<unsigned short> indices;

	// Ambient coefficient
	DirectX::XMFLOAT3 ambient = { 1,1,1 };
	// Diffuse coefficient
	DirectX::XMFLOAT3 diffuse = { 1,1,1 };
	// Texture metadata
	DirectX::TexMetadata metadata = {};
	// Scratch image
	DirectX::ScratchImage scratchImg = {};

	// Albedo
	DirectX::XMFLOAT3 baseColor = { 1,1,1 };
	// Metallic degree (0 or 1)
	float metalness = 0.0f;
	// Specular reflectivity (0 ~ 1)
	float specular = 0.5f;
	// Roughness
	float roughness = 0.0f;

	// Vertex Buffer
	ComPtr<ID3D12Resource> vertBuff;
	// Index Buffer
	ComPtr<ID3D12Resource> indexBuff;
	// Texture Buffer
	ComPtr<ID3D12Resource> texBuff;
	// Vertex Buffer View
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	// Index Buffer View
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	// SRV descriptor heap
	ComPtr<ID3D12DescriptorHeap> descHeapSRV;
};