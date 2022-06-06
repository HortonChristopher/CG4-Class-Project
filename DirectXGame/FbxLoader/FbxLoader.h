#pragma once

#include "fbxsdk.h"
#include "FbxModel.h"

#include <d3d12.h>
#include <d3dx12.h>
#include <string>

class FbxLoader
{

private: // エイリアス alias
	// std::を省略
	using string = std::string;

public: // 定数
	// モデル格納ルートパス Model storage root path
	static const string baseDirectory;

	// テクスチャがない場合の標準テクスチャファイル名 Standard texture file name when there is no texture
	static const string defaultTextureFileName;

public: // 静的メンバ関数 Static member function
	// シングルトンインスタンスの取得 Get a singleton instance
	static FbxLoader *GetInstance();

	// FBXの行列をXMMatrixに変換 Convert FBX matrix to XMMatrix
	static void ConvertMatrixFromFbx( DirectX::XMMATRIX *dst, const FbxAMatrix &src );

public: // メンバ関数 Member function
	void Initialize( ID3D12Device *device );

	void Finalize();
	
	FbxModel* LoadModelFromFile( const string &modelName );

	// 再帰的にノード構成を解析 Recursively analyze node configuration
	void ParseNodeRecursive( FbxModel *fbxmodel, FbxNode *fbxNode, Node* parent = nullptr );

	// メッシュ読み取り Mesh read
	void ParseMesh( FbxModel *fbxmodel, FbxNode *fbxNode );

	// 頂点座標読み取り Read vertex coordinates
	void ParseMeshVertices( FbxModel *fbxmodel, FbxMesh *fbxMesh );

	// 面情報読み取り Read surface information
	void ParseMeshFaces( FbxModel *fbxmodel, FbxMesh *fbxMesh );

	// マテリアル読み取り Material reading
	void ParseMaterial( FbxModel *fbxmodel, FbxNode *fbxNode );

	// スキニング情報読み取り Reading skinning information
	void ParseSkin( FbxModel *fbxmodel, FbxMesh *fbxMesh );

	// テクスチャ読み取り Texture reading
	void LoadTexture( FbxModel *fbxmodel, const std::string &fullpath );

	// ディレクトリを含んだファイルパスからファイル名を抽出する Extract filenames from file paths that include directories
	std::string ExtractFileName( const std::string &path );

private: // メンバ変数 Member variables
	// D3D12デバイス D3D12 device
	ID3D12Device *device = nullptr;

	// FBXマネージャー FBX manager
	FbxManager *fbxManager = nullptr;

	// FBXインポータ FBX importer
	FbxImporter *fbxImporter = nullptr;

	// privateなコンストラクタ（シングルトンパターン） private constructor (singleton pattern)
	FbxLoader() = default;
	// privateなデストラクタ（シングルトンパターン） Private destructor (singleton pattern)
	~FbxLoader() = default;
	// コピーコンストラクタを禁止（シングルトンパターン） Prohibit copy constructor (singleton pattern)
	FbxLoader(const FbxLoader& obj) = delete;
	// コピー代入演算子を禁止（シングルトンパターン） Prohibit copy assignment operator (singleton pattern)
	void operator=(const FbxLoader& obj) = delete;
};