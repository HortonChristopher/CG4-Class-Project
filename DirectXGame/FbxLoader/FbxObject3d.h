#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <d3dx12.h>
#include <string>

#include "FbxModel.h"
#include "Camera.h"

class FbxObject3d
{
protected: // エイリアス alias
// Microsoft::WRL::を省略
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;
	// DirectX::を省略
	using XMFLOAT2 = DirectX::XMFLOAT2;
	using XMFLOAT3 = DirectX::XMFLOAT3;
	using XMFLOAT4 = DirectX::XMFLOAT4;
	using XMMATRIX = DirectX::XMMATRIX;

public: // 定数 constant
// ボーンの最大数 Maximum number of bones
	static const int MAX_BONES = 32;

public: // サブクラス Subclass
// 定数バッファ用データ構造体(座標変換行列用) Data structure for constant buffer (for coordinate transformation matrix)
	struct ConstBufferDataTransform
	{
		XMMATRIX viewproj;
		XMMATRIX world;
		XMFLOAT3 cameraPos;
	};

	// 定数バッファ用データ構造体(スキニング) Data structure for constant buffer (skinning)
	struct ConstBufferDataSkin
	{
		XMMATRIX bones[MAX_BONES];
	};


public: // 静的メンバ関数 Static member function
// setter
	static void SetDevice( ID3D12Device *device ) { FbxObject3d::device = device; }
	static void SetCamera( Camera *camera ) { FbxObject3d::camera = camera; }

	/// <summary>
	/// グラフィックパイプラインの生成 Generate graphic pipeline
	/// </summary>
	static void CreateGraphicsPipeline();

private: // 静的メンバ変数 Static member variables
	static ID3D12Device *device;

	// カメラ camera
	static Camera *camera;

	// ルートシグネチャ Route signature
	static ComPtr<ID3D12RootSignature> rootSignature;
	// パイプラインステートオブジェクト Pipeline state object
	static ComPtr<ID3D12PipelineState> pipelineState;

	// 定数バッファ(スキン) Constant buffer (skin)
	ComPtr<ID3D12Resource> constBuffSkin;

public: // メンバ関数 Member function
	// 初期化 Initialization
	void Initialize();

	// 毎フレーム処理 Every frame processing
	void Update();
	
	// モデルのセット Set model
	void SetModel( FbxModel *fbxmodel ) { this->fbxmodel = fbxmodel; }
	
	// 描画 drawing
	void Draw( ID3D12GraphicsCommandList *cmdList );
	
	// 座標の取得 Get coordinates
	const XMFLOAT3 &GetPosition() { return position; }
	
	// 座標の設定 Coordinate setting
	void SetPosition( XMFLOAT3 position ) { this->position = position; }

	void SetRotation( XMFLOAT3 rotation ) { this->rotation = rotation; }
	
	// スケールの設定 Scale setting
	void SetScale( XMFLOAT3 scale ) { this->scale = scale; }
	
	// アニメーション開始 Animation start
	void PlayAnimation();

protected: // メンバ変数 Member variables
// 定数バッファ Constant buffer
	ComPtr<ID3D12Resource> constBuffTransform;

	// ローカルスケール Local scale
	XMFLOAT3 scale = { 1,1,1 };
	// X,Y,Z軸回りのローカル回転角 Local rotation angle around the X, Y, Z axes
	XMFLOAT3 rotation = { 0,0,0 };
	// ローカル座標 Local coordinates
	XMFLOAT3 position = { 5,0,0 };
	// ローカルワールド変換行列 Local world transformation matrix
	XMMATRIX matWorld;
	// モデル model
	FbxModel *fbxmodel = nullptr;

	// 1フレーム 1 frame
	FbxTime frameTime;
	// アニメーション開始時間 Animation start time
	FbxTime startTime;
	// アニメーション終了時間 Animation end time
	FbxTime endTime;
	// 現在時間 Current time
	FbxTime currentTime;
	// アニメーション再生中 Animation is playing
	bool isPlay = false;
};

