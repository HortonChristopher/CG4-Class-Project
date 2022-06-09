#pragma once

#include "fbxsdk.h"

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

public:
	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	/// <returns>インスタンス</returns>
	static FbxLoader* GetInstance();

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
	void LoadModelFromFile(const string& modelName);

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