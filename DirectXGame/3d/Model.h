#pragma once

#include <string>
#include <vector>
#include <DirectXMath.h>
#include <DirectXTex.h>

class Model
{
public: // Subclass
	// Vertex data structure
	struct VertexPosNormalUv
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT3 uv;
	};

public:
	// Friend Class
	friend class FbxLoader;

private:
	// Model Name
	std::string name;

	// Node Array
	std::vector<Node> nodes;

	// Node with mesh
	Node* meshNode = nullptr;

	// Vertex data array
	std::vector<VertexPosNormalUv> vertices;

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
};

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