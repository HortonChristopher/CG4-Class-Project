#pragma once

#include "fbxsdk.h"
#include "Model.h"

#include <d3d12.h>
#include <d3dx12.h>
#include <string>

class FbxLoader
{
private:
	// std abbreviation
	using string = std::string;

public:
	// Model Storage Route Bus
	static const string baseDirectory;

	// Standard texture file name when there is no texture
	static const string defaultTextureFileName;

public:
	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	/// <returns>インスタンス</returns>
	static FbxLoader* GetInstance();

	/// <summary>
	/// Convert FBX matrix to XMMatrix
	/// </summary>
	/// <param name="dst">Write destination</param>
	/// <param name="src">Original FBX matrix</param>
	static void ConvertMatrixFromFbx(DirectX::XMMATRIX* dst, const FbxAMatrix& src);

	/// <summary>
	/// Initialization
	/// </summary>
	/// <param name="device">D3D12Device</param>
	void Initialize(ID3D12Device* device);

	/// <summary>
	/// Clean Up
	/// </summary>
	void Finalize();

	/// <summary>
	/// Load FBX model from file
	/// </summary>
	/// <param name="modelName">Model Name</param>
	//void LoadModelFromFile(const string& modelName);
	Model* LoadModelFromFile(const string& modelName);

	/// <summary>
	/// Recursively analyze node configuration
	/// </summary>
	/// <param name="model">Import destination model object</param>
	/// <param name="fbxNode">Nodes to be analyzed</param>
	void ParseNodeRecursive(Model* model, FbxNode* fbxNode, Node* parent = nullptr);

	/// <summary>
	/// Reading of mesh
	/// </summary>
	/// <param name="model">Imported Model Object</param>
	/// <param name="fbxNode">Node to be analyzed</param>
	void ParseMesh(Model* model, FbxNode* fbxNode);

	// Vertex coordinate reading
	void ParseMeshVertices(Model* model, FbxMesh* fbxMesh);

	// Surface information reading
	void ParseMeshFaces(Model* model, FbxMesh* fbxMesh);

	// Material reading
	void ParseMaterial(Model* model, FbxNode* fbxNode);

	// Read Skinning Information
	void ParseSkin(Model* model, FbxMesh* fbxMesh);

	// Texture reading
	void LoadTexture(Model* model, const std::string& fullpath);

	std::string ExtractFileName(const std::string& path);

private:
	// privateなコンストラクタ（シングルトンパターン）
	FbxLoader() = default;
	// privateなデストラクタ（シングルトンパターン）
	~FbxLoader() = default;
	// コピーコンストラクタを禁止（シングルトンパターン）
	FbxLoader(const FbxLoader& obj) = delete;
	// コピー代入演算子を禁止（シングルトンパターン）
	void operator=(const FbxLoader& obj) = delete;

	// D3D12Device
	ID3D12Device* device = nullptr;
	// FBXManager
	FbxManager* fbxManager = nullptr;
	// FBXImporter
	FbxImporter* fbxImporter = nullptr;
};