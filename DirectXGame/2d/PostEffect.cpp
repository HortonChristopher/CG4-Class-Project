#include "PostEffect.h"
#include "WinApp.h"

#include <d3dx12.h>

using namespace DirectX;

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
	HRESULT result;

	// Base class initialization
	Sprite::Initialize();

	// Texture resource settings
	CD3DX12_RESOURCE_DESC texresDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		WinApp::window_width,
		(UINT)WinApp::window_height,
		1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
	);

	// Texture Buffer generation
	result = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0),
		D3D12_HEAP_FLAG_NONE,
		&texresDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texBuff));

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
	for (int i = 0; i < pixelCount; i++) { img[i] = 0xff0000ff; }

	// Transfer data to texture buffer
	result = texBuff->WriteToSubresource(0, nullptr, img, rowPitch, depthPitch);
	assert(SUCCEEDED(result));
	delete[] img;

	// Descriptor heap setting for SRV
	D3D12_DESCRIPTOR_HEAP_DESC srvDescHeapDesc = {};
	srvDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvDescHeapDesc.NumDescriptors = 1;

	// Generate descriptor heap for SRV
	result = device->CreateDescriptorHeap(&srvDescHeapDesc, IID_PPV_ARGS(&descHeapSRV));
	assert(SUCCEEDED(result));

	// SRV setting
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{}; // Configuration settings
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2D texture
	srvDesc.Texture2D.MipLevels = 1;

	// Create SRV in descriptor heap
	device->CreateShaderResourceView(texBuff.Get(), // Buffer associated with view
		&srvDesc,
		descHeapSRV->GetCPUDescriptorHandleForHeapStart()
	);
}

void PostEffect::Draw(ID3D12GraphicsCommandList* cmdList)
{
	// World matrix update
	this->matWorld = XMMatrixIdentity();
	this->matWorld *= XMMatrixRotationZ(XMConvertToRadians(rotation));
	this->matWorld *= XMMatrixTranslation(position.x, position.y, 0.0f);

	// Data transfer to a constant buffer
	ConstBufferData* constMap = nullptr;
	HRESULT result = this->constBuff->Map(0, nullptr, (void**)&constMap);
	if (SUCCEEDED(result)) {
		constMap->color = this->color;
		constMap->mat = this->matWorld * matProjection;	// Matrix composition
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
	cmdList->SetGraphicsRootDescriptorTable(1, descHeapSRV->GetGPUDescriptorHandleForHeapStart());
	// Drawing command
	cmdList->DrawInstanced(4, 1, 0, 0);
}