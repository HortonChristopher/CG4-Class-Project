#pragma once

#include <string>
#include <DirectXMath.h>
#include <vector>
#include <DirectXTex.h>
#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <fbxsdk.h>

struct Node
{
	// 名前 name
	std::string name;

	// ローカルスケール Local scale
	DirectX::XMVECTOR scaling = { 1,1,1,0 };

	// ローカル回転角 Local angle of rotation
	DirectX::XMVECTOR rotation = { 0,0,0,0 };

	// ローカル移動 Local move
	DirectX::XMVECTOR translation = { 0,0,0,1 };

	// ローカル変形行列 Local transformation matrix
	DirectX::XMMATRIX transform;

	// グローバル変形行列 Global transformation matrix
	DirectX::XMMATRIX globalTransform;

	// 親ノード Parent node
	Node *parent = nullptr;
};



class FbxModel
{
private: // エイリアス alias
// Microsoft::WRL::を省略
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;
	// DirectX::を省略
	using XMFLOAT2 = DirectX::XMFLOAT2;
	using XMFLOAT3 = DirectX::XMFLOAT3;
	using XMFLOAT4 = DirectX::XMFLOAT4;
	using XMMATRIX = DirectX::XMMATRIX;
	using TexMetadata = DirectX::TexMetadata;
	using ScratchImage = DirectX::ScratchImage;

	// std::を省略
	using string = std::string;
	template <class T> using vector = std::vector<T>;

public: // 定数 constant
// ボーンインデックスの最大数 Maximum number of bone indexes
	static const int MAX_BONE_INDICES = 4;

public: // サブクラス Subclass
// 頂点データ構造体 Vertex data structure
	struct VertexPosNormalUvSkin
	{
		DirectX::XMFLOAT3 pos; // xyz座標 xyz coordinates
		DirectX::XMFLOAT3 normal; // 法線ベクトルNormal vector
		DirectX::XMFLOAT2 uv; // uv座標 uv coordinates
		UINT boneIndex[MAX_BONE_INDICES]; // ボーン 番号 Bone number
		float boneWeight[MAX_BONE_INDICES]; // ボーン 重み Bone weight
	};

public:
	// フレンドクラス Friend class
	friend class FbxLoader;

	struct Bone
	{
		// 名前 name
		std::string name;

		// 初期姿勢の逆行列 Inverse matrix of initial posture
		DirectX::XMMATRIX invInitialPose;

		// クラスター(FBX側のボーン情報) Cluster (bone information on the FBX side)
		FbxCluster *fbxCluster;

		// コンストラクタ constructor
		Bone( const std::string &name )
		{
			this->name = name;
		}
	};

public:
	// 描画 drawing
	void Draw( ID3D12GraphicsCommandList *cmdList );

private: // メンバ変数 Member variables
	// モデル名 Model name
	std::string name;

	// ノード配列 Node array
	std::vector<Node> nodes;
	// メッシュを持つノード Node with mesh
	Node *meshNode = nullptr;
	// 頂点バッファVertex buffer
	ComPtr<ID3D12Resource> vertBuff;
	// インデックスバッファ Index buffer
	ComPtr<ID3D12Resource> indexBuff;
	// テクスチャバッファ Texture buffer
	ComPtr<ID3D12Resource> texBuff;
	// 頂点バッファビュー Vertex buffer view
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	// インデックスバッファビュー Index buffer view
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	// SRV用デスクリプタヒープ Descriptor heap for SRV
	ComPtr<ID3D12DescriptorHeap> descHeapSRV;
	// 頂点データ配列 Vertex data array
	std::vector<VertexPosNormalUvSkin> vertices;
	// 頂点インデックス配列 Vertex index array
	std::vector<unsigned short> indices;

	// アンビエント係数 Ambient coefficient
	DirectX::XMFLOAT3 ambient = { 1,1,1 };
	// ディフューズ係数 Diffuse coefficient
	DirectX::XMFLOAT3 diffuse = { 1,1,1 };
	// テクスチャメタデータ Texture metadata
	DirectX::TexMetadata metadata = {};
	// スクラッチイメージ Scratch image
	DirectX::ScratchImage scratchImg = {};

	// ボーン配列 Bone array
	std::vector<Bone> bones;

	// FBXシーン FBX scene
	FbxScene *fbxScene = nullptr;

public:
	// バッファ生成 Buffer generation
	void CreatBuffers( ID3D12Device *device );

	// モデルの変形行列取得 Get model transformation matrix
	const XMMATRIX &GetModelTransform() { return meshNode->globalTransform; }

	// getter
	std::vector<Bone> &GetBones() { return bones; }

	// getter
	FbxScene *GetFbxScene() { return fbxScene; }

	// デストラクタ Destructor
	~FbxModel();
};

