#include "PostEffect.h"

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

	ID3D12DescriptorHeap* ppHeaps[] = { descHeap.Get() };
	// Set descriptor heap
	cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	// Set constant buffer view
	cmdList->SetGraphicsRootConstantBufferView(0, this->constBuff->GetGPUVirtualAddress());
	// Set shader resource view
	cmdList->SetGraphicsRootDescriptorTable(1, CD3DX12_GPU_DESCRIPTOR_HANDLE(descHeap->GetGPUDescriptorHandleForHeapStart(), this->texNumber, descriptorHandleIncrementSize));
	// Drawing command
	cmdList->DrawInstanced(4, 1, 0, 0);
}