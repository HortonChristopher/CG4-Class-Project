#include "FbxModel.h"

void FbxModel::Draw( ID3D12GraphicsCommandList *cmdList )
{
	// 頂点バッファをセット Set vertex buffer
	cmdList->IASetVertexBuffers( 0, 1, &vbView );
	// インデックスバッファをセット Set index buffer
	cmdList->IASetIndexBuffer( &ibView );

	// デスクリプタヒープの配列 Array of descriptor heaps
	if ( descHeapSRV ) {
		ID3D12DescriptorHeap *ppHeaps[] = { descHeapSRV.Get() };
		cmdList->SetDescriptorHeaps( _countof( ppHeaps ), ppHeaps );
	}
	// シェーダリソースビューをセット Set shader resource view
	cmdList->SetGraphicsRootDescriptorTable( 1, descHeapSRV->GetGPUDescriptorHandleForHeapStart() );

	// 描画コマンド Drawing command
	cmdList->DrawIndexedInstanced( (UINT)indices.size(), 1, 0, 0, 0 );
}

FbxModel::~FbxModel()
{
	// FBXシーンの解放 Release the FBX scene
	fbxScene->Destroy();
}

void FbxModel::CreatBuffers( ID3D12Device *device )
{
	HRESULT result;

	// 頂点データの全体サイズ Overall size of vertex data
	UINT sizeVB = static_cast<UINT>(sizeof( VertexPosNormalUvSkin ) * vertices.size());

	// 頂点バッファ生成 Vertex buffer generation
	result = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD ),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer( sizeVB ),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS( &vertBuff ) );

	// 頂点バッファへのデータ転送 Data transfer to vertex buffer
	VertexPosNormalUvSkin *vertMap = nullptr;
	result = vertBuff->Map( 0, nullptr, (void **)&vertMap );
	if ( SUCCEEDED( result ) )
	{
		std::copy( vertices.begin(), vertices.end(), vertMap );
		vertBuff->Unmap( 0, nullptr );
	}

	// 頂点バッファビュー(VBV)の作成 Creating a vertex buffer view (VBV)
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
	vbView.SizeInBytes = sizeVB;
	vbView.StrideInBytes = sizeof( vertices[0] );

	// 頂点インデックス全体のサイズ Overall size of vertex index
	UINT sizeIB = static_cast<UINT>(sizeof( unsigned short ) * indices.size());

	// インデックスバッファ生成 Index buffer generation
	result = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD ),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer( sizeIB ),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS( &indexBuff ) );

	// インデックスバッファへのデータ転送 Data transfer to index buffer
	unsigned short *indexMap = nullptr;
	result = indexBuff->Map( 0, nullptr, (void **)&indexMap );
	if ( SUCCEEDED( result ) )
	{
		std::copy( indices.begin(), indices.end(), indexMap );
		indexBuff->Unmap( 0, nullptr );
	}

	// インデックスバッファビュー(IBV)の作成 Creating an index buffer view (IBV)
	ibView.BufferLocation = indexBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeIB;

	// テクスチャ画像データ Texture image data
	const DirectX::Image *img = scratchImg.GetImage( 0, 0, 0 ); // 生データ抽出 Raw data extraction
	assert( img ); 

	// リソース設定 Resource settings
	CD3DX12_RESOURCE_DESC texresDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format,
		metadata.width,
		(UINT)metadata.height,
		(UINT16)metadata.arraySize,
		(UINT16)metadata.mipLevels
	);

	// テクスチャ用バッファの生成 Generate buffer for texture
	result = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES( D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
			D3D12_MEMORY_POOL_L0 ),
		D3D12_HEAP_FLAG_NONE,
		&texresDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // テクスチャ用指定 Specification for texture
		nullptr,
		IID_PPV_ARGS( &texBuff ) );

	// テクスチャバッファにデータ転送 Data transfer to texture buffer
	result = texBuff->WriteToSubresource(
		0,
		nullptr, // 全領域へのコピー Copy to all areas
		img->pixels, // 元データアドレス Original data address
		(UINT)img->rowPitch, // 1ラインサイズ 1 line size
		(UINT)img->slicePitch // 1枚サイズ 1 sheet size
	);

	// SRV用デスクリプタヒープを生成 Generate descriptor heap for SRV
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // シェーダから見えるように As visible from the shader
	descHeapDesc.NumDescriptors = 1; // テクスチャ枚数 Number of textures
	result = device->CreateDescriptorHeap( &descHeapDesc, IID_PPV_ARGS( &descHeapSRV ) ); // 生成 generation

	// シェーダリソースビュー(SRV)生成 Shader resource view (SRV) generation
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{}; // 設定構造体 Setting structure
	D3D12_RESOURCE_DESC resDesc = texBuff->GetDesc();

	srvDesc.Format = resDesc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ 2D texture
	srvDesc.Texture2D.MipLevels = 1;

	device->CreateShaderResourceView( texBuff.Get(), // ビューと関連付けるバッファ Buffer associated with view
		&srvDesc,// テクスチャ設定情報 Texture setting information
		descHeapSRV->GetCPUDescriptorHandleForHeapStart() // ヒープの先頭アドレス Heap start address
	);
}
