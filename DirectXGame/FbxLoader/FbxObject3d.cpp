#include "FbxObject3d.h"
#include "FbxLoader.h"

#include<d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

using namespace Microsoft::WRL;
using namespace DirectX;

ID3D12Device *FbxObject3d::device = nullptr;
Camera *FbxObject3d::camera = nullptr;

ComPtr<ID3D12RootSignature> FbxObject3d::rootSignature;
ComPtr<ID3D12PipelineState> FbxObject3d::pipelineState;

void FbxObject3d::CreateGraphicsPipeline()
{
	HRESULT result = S_FALSE;
	ComPtr<ID3DBlob> vsBlob; // 頂点シェーダオブジェクト Vertex shader object
	ComPtr<ID3DBlob> psBlob;    // ピクセルシェーダオブジェクト Pixel shader object
	ComPtr<ID3DBlob> errorBlob; // エラーオブジェクト Error object

	assert( device );

	// 頂点シェーダの読み込みとコンパイル Loading and compiling vertex shaders
	result = D3DCompileFromFile(
		L"Resources/shaders/FBXVS.hlsl",    // シェーダファイル名 Shader file name
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルード可能にする Enable to include
		"main", "vs_5_0",    // エントリーポイント名、シェーダーモデル指定 Entry point name, shader model specification
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用設定 Debug settings
		0,
		&vsBlob, &errorBlob );
	if ( FAILED( result ) ) {
		// errorBlobからエラー内容をstring型にコピー Copy the error content from errorBlob to string type
		std::string errstr;
		errstr.resize( errorBlob->GetBufferSize() );

		std::copy_n( (char *)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			errstr.begin() );
		errstr += "\n";
		// エラー内容を出力ウィンドウに表示 Display error details in output window
		OutputDebugStringA( errstr.c_str() );
		exit( 1 );
	}

	// ピクセルシェーダの読み込みとコンパイル Loading and compiling pixel shaders
	result = D3DCompileFromFile(
		L"Resources/shaders/FBXPS.hlsl",    // シェーダファイル名 Shader file name
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルード可能にする Enable to include
		"main", "ps_5_0",    // エントリーポイント名、シェーダーモデル指定 Entry point name, shader model specification
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用設定 Debug settings
		0,
		&psBlob, &errorBlob );
	if ( FAILED( result ) ) {
		// errorBlobからエラー内容をstring型にコピー Copy the error content from errorBlob to string type
		std::string errstr;
		errstr.resize( errorBlob->GetBufferSize() );

		std::copy_n( (char *)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			errstr.begin() );
		errstr += "\n";
		// エラー内容を出力ウィンドウに表示 Display error details in output window
		OutputDebugStringA( errstr.c_str() );
		exit( 1 );
	}

	// 頂点レイアウト Vertex layout
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ // xy座標(1行で書いたほうが見やすい) xy coordinates (it is easier to see if written in one line)
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{ // 法線ベクトル(1行で書いたほうが見やすい) Normal vector (easier to see if written in one line)
			"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{ // uv座標(1行で書いたほうが見やすい) uv coordinates (it is easier to see if written in one line)
			"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{ // 影響を受けるボーン番号(4つ) Affected bone numbers (4)
			"BONEINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{ // ボーンのスキンウェイト(4つ) Bone skin weights (4)
			"BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
	};

	// グラフィックスパイプラインの流れを設定 Set the flow of the graphics pipeline
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline{};
	gpipeline.VS = CD3DX12_SHADER_BYTECODE( vsBlob.Get() );
	gpipeline.PS = CD3DX12_SHADER_BYTECODE( psBlob.Get() );

	// サンプルマスク Sample mask
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // 標準設定
	// ラスタライザステート Rasterizer state
	gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC( D3D12_DEFAULT );
	//gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	//gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	// デプスステンシルステート Depth stencil state
	gpipeline.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC( D3D12_DEFAULT );

	// レンダーターゲットのブレンド設定 Render target blend settings
	D3D12_RENDER_TARGET_BLEND_DESC blenddesc{};
	blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;    // RBGA全てのチャンネルを描画 RBGA Draw all channels
	blenddesc.BlendEnable = true;
	blenddesc.BlendOp = D3D12_BLEND_OP_ADD;
	blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;

	blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	
	// ブレンドステートの設定 Blend state setting
	gpipeline.BlendState.RenderTarget[0] = blenddesc;

	// 深度バッファのフォーマット Depth buffer format
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	// 頂点レイアウトの設定 Vertex layout settings
	gpipeline.InputLayout.pInputElementDescs = inputLayout;
	gpipeline.InputLayout.NumElements = _countof( inputLayout );

	// 図形の形状設定（三角形） Shape shape setting (triangle)
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	gpipeline.NumRenderTargets = 1;    // 描画対象は1つ One drawing target
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 0～255指定のRGBA RGBA specified from 0 to 255
	gpipeline.SampleDesc.Count = 1; // 1ピクセルにつき1回サンプリング Sampling once per pixel

	// デスクリプタレンジ Descriptor range
	CD3DX12_DESCRIPTOR_RANGE descRangeSRV;
	descRangeSRV.Init( D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 ); // t0 レジスタ t0 register

	// ルートパラメータ Route parameters
	CD3DX12_ROOT_PARAMETER rootparams[3];
	// CBV（座標変換行列用） CBV (for coordinate transformation matrix)
	rootparams[0].InitAsConstantBufferView( 0, 0, D3D12_SHADER_VISIBILITY_ALL );
	// SRV（テクスチャ） SRV (texture)
	rootparams[1].InitAsDescriptorTable( 1, &descRangeSRV, D3D12_SHADER_VISIBILITY_ALL );
	// CBV (スキニング用) CBV (for skinning)
	rootparams[2].InitAsConstantBufferView( 3, 0, D3D12_SHADER_VISIBILITY_ALL );
	// スタティックサンプラー Static sampler
	CD3DX12_STATIC_SAMPLER_DESC samplerDesc = CD3DX12_STATIC_SAMPLER_DESC( 0 );

	// ルートシグネチャの設定Route signature settings
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_0( _countof( rootparams ), rootparams, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT );

	ComPtr<ID3DBlob> rootSigBlob;
	// バージョン自動判定のシリアライズ Serialization of automatic version judgment
	result = D3DX12SerializeVersionedRootSignature( &rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob );
	// ルートシグネチャの生成 Route signature generation
	result = device->CreateRootSignature( 0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS( rootSignature.ReleaseAndGetAddressOf() ) );
	if ( FAILED( result ) ) { assert( 0 ); }

	gpipeline.pRootSignature = rootSignature.Get();

	// グラフィックスパイプラインの生成 Graphics pipeline generation
	result = device->CreateGraphicsPipelineState( &gpipeline, IID_PPV_ARGS( pipelineState.ReleaseAndGetAddressOf() ) );
	if ( FAILED( result ) ) { assert( 0 ); }
}

void FbxObject3d::Initialize()
{
	HRESULT result;
	// 定数バッファの生成 Generate constant buffer
	result = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD ), 	// アップロード可能 Uploadable
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer( (sizeof( ConstBufferDataTransform ) + 0xff) & ~0xff ),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS( &constBuffTransform ) );

	// スキニング情報定数バッファ生成 Skinning information constant buffer generation
	result = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD ), 	// アップロード可能 Uploadable
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer( (sizeof( ConstBufferDataSkin ) + 0xff) & ~0xff ),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS( &constBuffSkin ) );

	// 定数バッファへデータ転送 Data transfer to a constant buffer
	ConstBufferDataSkin *constMapSkin = nullptr;
	result = constBuffSkin->Map( 0, nullptr, (void **)&constMapSkin );
	for ( int i = 0; i < MAX_BONES; i++ )
	{
		constMapSkin->bones[i] = XMMatrixIdentity();
	}
	constBuffSkin->Unmap( 0, nullptr );

	// 1フレーム分の時間を60FPSで設定 Set the time for one frame at 60 FPS
	frameTime.SetTime( 0, 0, 0, 1, 0, FbxTime::EMode::eFrames60 );
}

void FbxObject3d::Update()
{
	XMMATRIX matScale, matRot, matTrans;

	// スケール、回転、平行移動行列の計算 Calculation of scale, rotation, translation matrix
	matScale = XMMatrixScaling( scale.x, scale.y, scale.z );
	matRot = XMMatrixIdentity();
	matRot *= XMMatrixRotationZ( XMConvertToRadians( rotation.z ) );
	matRot *= XMMatrixRotationX( XMConvertToRadians( rotation.x ) );
	matRot *= XMMatrixRotationY( XMConvertToRadians( rotation.y ) );
	matTrans = XMMatrixTranslation( position.x, position.y, position.z );

	// ワールド行列の合成 World matrix composition
	matWorld = XMMatrixIdentity(); // 変形をリセット Reset transformation
	matWorld *= matScale; // ワールド行列にスケーリングを反映 Reflect scaling in world matrix
	matWorld *= matRot; // ワールド行列に回転を反映 Reflect the rotation in the world matrix
	matWorld *= matTrans; // ワールド行列に平行移動を反映 Reflect translation in world matrix

	// ビュープロダクション行列 View production matrix
	const XMMATRIX &matViewProjection = camera->GetViewProjectionMatrix();

	// モデルのメッシュトランスフォーム Model mesh transform
	const XMMATRIX &modelTransform = fbxmodel->GetModelTransform();

	// カメラ座標 Camera coordinates
	const XMFLOAT3 &cameraPos = camera->GetEye();

	HRESULT result;

	// 定数バッファへデータ転送 Data transfer to a constant buffer
	ConstBufferDataTransform *constMap = nullptr;
	result = constBuffTransform->Map( 0, nullptr, (void **)&constMap );
	if ( SUCCEEDED( result ) )
	{
		constMap->viewproj = matViewProjection;
		constMap->world = modelTransform * matWorld;
		constMap->cameraPos = cameraPos;
		constBuffTransform->Unmap( 0, nullptr );
	}

	// ボーン配列 Bone array
	std::vector<FbxModel::Bone> &bones = fbxmodel->GetBones();

	if ( isPlay == false )
	{
		PlayAnimation();
	}

	// アニメーション animation
	if ( isPlay )
	{
		// 1フレーム進める Advance one frame
		currentTime += frameTime;
		// 最後まで再生したら先頭に戻す After playing to the end, return to the beginning
		if ( currentTime > endTime )
		{
			currentTime = startTime;
		}
	}


	// 定数バッファデータ転送 Constant buffer data transfer
	ConstBufferDataSkin *constMapSkin = nullptr;
	result = constBuffSkin->Map( 0, nullptr, (void **)&constMapSkin );
	for ( int i = 0; i < bones.size(); i++ )
	{
		// 今の姿勢行列 Current posture matrix
		XMMATRIX matCurrentPose;
		// 今の姿勢行列を取得 Get the current posture matrix
		FbxAMatrix fbxCurrentPose =
			bones[i].fbxCluster->GetLink()->EvaluateGlobalTransform( currentTime );
		// XMMATRIXに変換 Convert to XMMATRIX
		FbxLoader::ConvertMatrixFromFbx( &matCurrentPose, fbxCurrentPose );
		// 合成してスキニング行列に Synthesize into a skinning matrix
		constMapSkin->bones[i] = bones[i].invInitialPose * matCurrentPose;
	}
	constBuffSkin->Unmap( 0, nullptr );
}

void FbxObject3d::Draw( ID3D12GraphicsCommandList *cmdList )
{
	// モデルの割り当てがなければ描画しない Do not draw without model assignment
	if ( fbxmodel == nullptr ) {
		return;
	}

	// パイプラインステートの設定 Setting the pipeline state
	cmdList->SetPipelineState( pipelineState.Get() );
	// ルートシグネチャの設定 Route signature settings
	cmdList->SetGraphicsRootSignature( rootSignature.Get() );
	// プリミティブ形状を設定 Set primitive shape
	cmdList->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	// 定数バッファビューをセット Set constant buffer view
	cmdList->SetGraphicsRootConstantBufferView( 0, constBuffTransform->GetGPUVirtualAddress() );
	// スキニング情報定数バッファビューをセット Set skinning information constant buffer view
	cmdList->SetGraphicsRootConstantBufferView( 2, constBuffSkin->GetGPUVirtualAddress() );

	// モデル描画 Model drawing
	fbxmodel->Draw( cmdList );
}

void FbxObject3d::PlayAnimation()
{
	FbxScene *fbxScene = fbxmodel->GetFbxScene();
	// 0番目のアニメーション取得 Get 0th animation
	FbxAnimStack *animstack = fbxScene->GetSrcObject<FbxAnimStack>( 0 );
	// アニメーションの名前取得 Get the name of the animation
	const char *animstackname = animstack->GetName();
	// アニメーションの時間情報 Animation time information
	FbxTakeInfo *takeinfo = fbxScene->GetTakeInfo( animstackname );

	// 開始時間取得 Get start time
	startTime = takeinfo->mLocalTimeSpan.GetStart();
	// 終了時間取得 Get end time
	endTime = takeinfo->mLocalTimeSpan.GetStop();
	// 開始時間に合わせる Match the start time
	currentTime = startTime;
	// 再生中状態にする Put it in the playing state
	isPlay = true;
}
