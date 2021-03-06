#include "PostEffect.h"
#include "WinApp.h"
#include "Input.h"

#include <d3dx12.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

// Member change entity
const float PostEffect::clearColor[4] = { 0.25f, 0.5f, 0.1f, 0.0f }; // RGBA

PostEffect::PostEffect() : Sprite(
	100, // Texture number
	{ 0, 0 }, // Coordinates
	{ 500.0f, 500.0f }, // Size
	{ 1,1,1,1 }, // Color
	{ 0.0f, 0.0f }, // Anchor point
	false, // Left/Right inversion flag
	false) // Up/Down inversion flag
{
}

void PostEffect::Initialize()
{
	// Pipeline Generation
	CreateGraphicsPipelineState();

	HRESULT result;

	// Base class initialization
	//Sprite::Initialize();

	// Vertex Buffer Generation
	result = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(VertexPosUv) * vertNum),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));
	assert(SUCCEEDED(result));

	// Vertex Data
	VertexPosUv vertices[vertNum] =
	{
		{{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f}}, // Lower Left
		{{-0.5f, +0.5f, 0.0f}, {0.0f, 0.0f}}, // Upper Left
		{{+0.5f, -0.5f, 0.0f}, {1.0f, 1.0f}}, // Lower Right
		{{+0.5f, +0.5f, 0.0f}, {1.0f, 0.0f}}, // Upper right
	};

	// Transfer data to the vertex buffer
	VertexPosUv* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	if (SUCCEEDED(result))
	{
		memcpy(vertMap, vertices, sizeof(vertices));
		vertBuff->Unmap(0, nullptr);
	}

	// Create the vertical buffer
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
	vbView.SizeInBytes = sizeof(VertexPosUv) * 4;
	vbView.StrideInBytes = sizeof(VertexPosUv);

	// Generate Constant Buffer
	result = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) && ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuff));
	assert(SUCCEEDED(result));

	// Texture resource settings
	CD3DX12_RESOURCE_DESC texresDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		WinApp::window_width,
		(UINT)WinApp::window_height,
		1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
	);

	for (int i = 0; i < 2; i++)
	{
		// Texture Buffer generation
		result = device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0),
			D3D12_HEAP_FLAG_NONE,
			&texresDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, clearColor),
			IID_PPV_ARGS(&texBuff[i]));
		assert(SUCCEEDED(result));

		// Clear texture in red
	//Number of pixels (1280 x 720 = 921600 pixels)
		const UINT pixelCount = WinApp::window_width * WinApp::window_height;

		// Data size for one line of image
		const UINT rowPitch = sizeof(UINT) * WinApp::window_width;

		// Data size of the whole image
		const UINT depthPitch = rowPitch * WinApp::window_height;

		// Image
		UINT* img = new UINT[pixelCount];
		for (int j = 0; j < pixelCount; j++) { img[j] = 0xff0000ff; }

		// Transfer data to texture buffer
		result = texBuff[i]->WriteToSubresource(0, nullptr, img, rowPitch, depthPitch);
		assert(SUCCEEDED(result));
		delete[] img;
	}

	// Descriptor heap setting for SRV
	D3D12_DESCRIPTOR_HEAP_DESC srvDescHeapDesc = {};
	srvDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvDescHeapDesc.NumDescriptors = 2;

	// Generate descriptor heap for SRV
	result = device->CreateDescriptorHeap(&srvDescHeapDesc, IID_PPV_ARGS(&descHeapSRV));
	assert(SUCCEEDED(result));

	// SRV setting
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{}; // Configuration settings
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2D texture
	srvDesc.Texture2D.MipLevels = 1;

	for (int i = 0; i < 2; i++)
	{
		// Create SRV in descriptor heap
		device->CreateShaderResourceView(texBuff[i].Get(), // Buffer associated with view
			&srvDesc,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(
				descHeapSRV->GetCPUDescriptorHandleForHeapStart(), i,
				device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
			)
		);
	}

	// Descriptor heap settings for RTV
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescHeapDesc{};
	rtvDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescHeapDesc.NumDescriptors = 2;

	// Generate descriptor heap for RTV
	result = device->CreateDescriptorHeap(&rtvDescHeapDesc, IID_PPV_ARGS(&descHeapRTV));
	assert(SUCCEEDED(result));

	for (int i = 0; i < 2; i++)
	{
		// Create RTV as descriptor heap
		device->CreateRenderTargetView(texBuff[i].Get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(
				descHeapRTV->GetCPUDescriptorHandleForHeapStart(), i,
				device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV))
		);
	}

	// Depth buffer resource settings
	CD3DX12_RESOURCE_DESC depthResDesc =
		CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_D32_FLOAT,
			WinApp::window_width,
			WinApp::window_height,
			1, 0,
			1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
		);

	// Generate depth buffer
	result = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0),
		IID_PPV_ARGS(&depthBuff));
	assert(SUCCEEDED(result));

	// Descriptor heap for DSV
	D3D12_DESCRIPTOR_HEAP_DESC DescHeapDesc{};
	DescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	DescHeapDesc.NumDescriptors = 1;

	// Create descriptor heap for DSV
	result = device->CreateDescriptorHeap(&DescHeapDesc, IID_PPV_ARGS(&descHeapDSV));
	assert(SUCCEEDED(result));

	// Create DSV in descriptor heap
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // Depth value format
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device->CreateDepthStencilView(depthBuff.Get(),
		&dsvDesc,
		descHeapDSV->GetCPUDescriptorHandleForHeapStart());
}

void PostEffect::CreateGraphicsPipelineState()
{
	HRESULT result = S_FALSE;
	ComPtr<ID3DBlob> vsBlob; // ???_?V?F?[?_?I?u?W?F?N?g
	ComPtr<ID3DBlob> psBlob;	// ?s?N?Z???V?F?[?_?I?u?W?F?N?g
	ComPtr<ID3DBlob> errorBlob; // ?G???[?I?u?W?F?N?g

	// ???_?V?F?[?_?????????????R???p?C??
	result = D3DCompileFromFile(
		L"Resources/shaders/PostEffectTestVS.hlsl",	// ?V?F?[?_?t?@?C????
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // ?C???N???[?h???\??????
		"main", "vs_5_0",	// ?G???g???[?|?C???g???A?V?F?[?_?[???f???w??
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // ?f?o?b?O?p????
		0,
		&vsBlob, &errorBlob);
	if (FAILED(result)) {
		// errorBlob?????G???[???e??string?^???R?s?[
		std::string errstr;
		errstr.resize(errorBlob->GetBufferSize());

		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			errstr.begin());
		errstr += "\n";
		// ?G???[???e???o???E?B???h?E???\??
		OutputDebugStringA(errstr.c_str());

		assert(0);
	}

	// ?s?N?Z???V?F?[?_?????????????R???p?C??
	result = D3DCompileFromFile(
		L"Resources/shaders/PostEffectTestPS.hlsl",	// ?V?F?[?_?t?@?C????
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // ?C???N???[?h???\??????
		"main", "ps_5_0",	// ?G???g???[?|?C???g???A?V?F?[?_?[???f???w??
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // ?f?o?b?O?p????
		0,
		&psBlob, &errorBlob);
	if (FAILED(result)) {
		// errorBlob?????G???[???e??string?^???R?s?[
		std::string errstr;
		errstr.resize(errorBlob->GetBufferSize());

		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			errstr.begin());
		errstr += "\n";
		// ?G???[???e???o???E?B???h?E???\??
		OutputDebugStringA(errstr.c_str());

		assert(0);
	}

	// ???_???C?A?E?g
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ // xy???W(1?s??????????????????????)
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{ // uv???W(1?s??????????????????????)
			"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
	};

	// ?O???t?B?b?N?X?p?C?v???C??????????????
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline{};
	gpipeline.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
	gpipeline.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());

	// ?T???v???}?X?N
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // ?W??????
	// ???X?^???C?U?X?e?[?g
	gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	// ?f?v?X?X?e???V???X?e?[?g
	gpipeline.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS; // ?????????????[??

	// ?????_?[?^?[?Q?b?g???u?????h????
	D3D12_RENDER_TARGET_BLEND_DESC blenddesc{};
	blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;	// RBGA?S?????`?????l?????`??
	blenddesc.BlendEnable = true;
	blenddesc.BlendOp = D3D12_BLEND_OP_ADD;
	blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;

	blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;

	// ?u?????h?X?e?[?g??????
	gpipeline.BlendState.RenderTarget[0] = blenddesc;

	// ?[?x?o?b?t?@???t?H?[?}?b?g
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	// ???_???C?A?E?g??????
	gpipeline.InputLayout.pInputElementDescs = inputLayout;
	gpipeline.InputLayout.NumElements = _countof(inputLayout);

	// ?}?`???`???????i?O?p?`?j
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	gpipeline.NumRenderTargets = 1;	// ?`????????1??
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 0?`255?w????RGBA
	gpipeline.SampleDesc.Count = 1; // 1?s?N?Z????????1???T???v?????O

	// ?f?X?N???v?^?????W
	CD3DX12_DESCRIPTOR_RANGE descRangeSRV0;
	descRangeSRV0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0 ???W?X?^
	CD3DX12_DESCRIPTOR_RANGE descRangeSRV1;
	descRangeSRV1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // t1 ???W?X?^

	// ???[?g?p?????[?^
	CD3DX12_ROOT_PARAMETER rootparams[3];
	rootparams[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
	rootparams[1].InitAsDescriptorTable(1, &descRangeSRV0, D3D12_SHADER_VISIBILITY_ALL);
	rootparams[2].InitAsDescriptorTable(1, &descRangeSRV1, D3D12_SHADER_VISIBILITY_ALL);

	// ?X?^?e?B?b?N?T???v???[
	CD3DX12_STATIC_SAMPLER_DESC samplerDesc = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_POINT); // s0 ???W?X?^

	// ???[?g?V?O?l?`????????
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_0(_countof(rootparams), rootparams, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSigBlob;
	// ?o?[?W???????????????V???A???C?Y
	result = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
	assert(SUCCEEDED(result));
	// ???[?g?V?O?l?`????????
	result = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(result));

	gpipeline.pRootSignature = rootSignature.Get();

	// ?O???t?B?b?N?X?p?C?v???C????????
	result = device->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&pipelineState));
	assert(SUCCEEDED(result));
}

void PostEffect::PreDrawScene(ID3D12GraphicsCommandList* cmdList)
{
	for (int i = 0; i < 2; i++)
	{
		// Change resource barrier (shader resource->drawable)
		cmdList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(texBuff[i].Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET));
	}

	// Get handle of descriptor heap for render target view
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHs[2];
	for (int i = 0; i < 2; i++)
	{
		rtvHs[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(
			descHeapRTV->GetCPUDescriptorHandleForHeapStart(), i,
			device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
		);
	}

	// Get handle of depth stencil view descriptor heap
	D3D12_CPU_DESCRIPTOR_HANDLE dsvH = descHeapDSV->GetCPUDescriptorHandleForHeapStart();

	// Set render target
	cmdList->OMSetRenderTargets(2, rtvHs, false, &dsvH);

	CD3DX12_VIEWPORT viewports[2];
	CD3DX12_RECT scissorRects[2];

	for (int i = 0; i < 2; i++)
	{
		viewports[i] = CD3DX12_VIEWPORT(0.0f, 0.0f, WinApp::window_width, WinApp::window_height);
		scissorRects[i] = CD3DX12_RECT(0, 0, WinApp::window_width, WinApp::window_height);
	}

	// Viewport settings
	cmdList->RSSetViewports(2, viewports);

	// Scissoring short form setting
	cmdList->RSSetScissorRects(2, scissorRects);

	for (int i = 0; i < 2; i++)
	{
		// Clear full screen
		cmdList->ClearRenderTargetView(rtvHs[i], clearColor, 0, nullptr);
	}

	// Clear depth buffer
	cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0,
		nullptr);
}

void PostEffect::Draw(ID3D12GraphicsCommandList* cmdList)
{
	// World matrix update
	//this->matWorld = XMMatrixIdentity();
	//this->matWorld *= XMMatrixRotationZ(XMConvertToRadians(rotation));
	//this->matWorld *= XMMatrixTranslation(position.x, position.y, 0.0f);

	// Data transfer to a constant buffer
	ConstBufferData* constMap = nullptr;
	HRESULT result = this->constBuff->Map(0, nullptr, (void**)&constMap);
	if (SUCCEEDED(result)) {
		constMap->color = this->color;
		//constMap->mat = this->matWorld * matProjection;	// Matrix composition
		constMap->mat = XMMatrixIdentity();
		this->constBuff->Unmap(0, nullptr);
	}

	// Pipeline state settings
	cmdList->SetPipelineState(pipelineState.Get());

	// Root signature settings
	cmdList->SetGraphicsRootSignature(rootSignature.Get());

	// Set the primitive shape
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// Vertex buffer settings
	cmdList->IASetVertexBuffers(0, 1, &this->vbView);

	//ID3D12DescriptorHeap* ppHeaps[] = { descHeap.Get() };
	ID3D12DescriptorHeap* ppHeaps[] = { descHeapSRV.Get() };
	// Set descriptor heap
	cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	// Set constant buffer view
	cmdList->SetGraphicsRootConstantBufferView(0, this->constBuff->GetGPUVirtualAddress());
	// Set shader resource view
	//cmdList->SetGraphicsRootDescriptorTable(1, CD3DX12_GPU_DESCRIPTOR_HANDLE(descHeap->GetGPUDescriptorHandleForHeapStart(), this->texNumber, descriptorHandleIncrementSize));
	//cmdList->SetGraphicsRootDescriptorTable(1, descHeapSRV->GetGPUDescriptorHandleForHeapStart());
	cmdList->SetGraphicsRootDescriptorTable(1,
		CD3DX12_GPU_DESCRIPTOR_HANDLE(
			descHeapSRV->GetGPUDescriptorHandleForHeapStart(), 0,
			device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
		)
	);
	cmdList->SetGraphicsRootDescriptorTable(2,
		CD3DX12_GPU_DESCRIPTOR_HANDLE(
			descHeapSRV->GetGPUDescriptorHandleForHeapStart(), 1,
			device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
		)
	);
	// Drawing command
	cmdList->DrawInstanced(4, 1, 0, 0);
}

void PostEffect::PostDrawScene(ID3D12GraphicsCommandList* cmdList)
{
	for (int i = 0; i < 2; i++)
	{
		// Change resource barrier (Drawable->Shader resource)
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texBuff[i].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}
}