#include "Model.h"

void Model::CreateBuffers(ID3D12Device* device)
{
	HRESULT result;

	// Overall size of vertex data
	UINT sizeVB = static_cast<UINT>(sizeof(VertexPosNormalUv) * vertices.size());

	// Vertex buffer generation
	result = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeVB),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));

	// Data transfer to vertex buffer
	VertexPosNormalUv* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	if (SUCCEEDED(result))
	{
		std::copy(vertices.begin(), vertices.end(), vertMap);
		vertBuff->Unmap(0, nullptr);
	}

	// Create vertex buffer view
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
	vbView.SizeInBytes = sizeVB;
	vbView.StrideInBytes = sizeof(vertices[0]);

	// Overall size of Index Buffer
	UINT sizeIB = static_cast<UINT>(sizeof(unsigned short) * indices.size());

	// Create Index Buffer
	result = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeIB),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuff));

	// Data transfer to index buffer
	unsigned short* indexMap = nullptr;
	result = indexBuff->Map(0, nullptr, (void**)&indexMap);
	if (SUCCEEDED(result))
	{
		std::copy(indices.begin(), indices.end(), indexMap);
		indexBuff->Unmap(0, nullptr);
	}

	// Create Index Buffer View
	ibView.BufferLocation = indexBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeIB;

	// Texture Image Data
	const DirectX::Image* img = scratchImg.GetImage(0, 0, 0); // Raw data extraction
	assert(img);

	// Resource settings
	CD3DX12_RESOURCE_DESC texresDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format,
		metadata.width,
		(UINT)metadata.height,
		(UINT16)metadata.arraySize,
		(UINT16)metadata.mipLevels
	);

	// Setting Texture Buffer
	result = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0),
		D3D12_HEAP_FLAG_NONE,
		&texresDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // Texture specifications
		nullptr,
		IID_PPV_ARGS(&texBuff));

	// Transfer Data to Texture Buffer
	result = texBuff->WriteToSubresource(
		0,
		nullptr, // copy to all areas
		img->pixels, // Original teledata address
		(UINT)img->rowPitch, // 1 line size
		(UINT)img->slicePitch // 1 sheet size
	);

	// Vertex Buffer Generation (Material)
	result = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferDataMaterial) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ, // Texture specifications
		nullptr,
		IID_PPV_ARGS(&constBuffMaterial));

	// Transfer Data to Vertex Buffer (Material)
	ConstBufferDataMaterial* constMapMaterial = nullptr;
	result = constBuffMaterial->Map(0, nullptr, (void**)&constMapMaterial);
	if (SUCCEEDED(result))
	{
		constMapMaterial->baseColor = baseColor;
		constMapMaterial->metalness = metalness;
		constMapMaterial->specular = specular;
		constMapMaterial->roughness = roughness;
		constBuffMaterial->Unmap(0, nullptr);
	}

	// SRV descriptor heap creation
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // As visible from the shader
	descHeapDesc.NumDescriptors = 1; // Number of textures
	result = device->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&descHeapSRV)); // Creation

	// Shader Resource View Creation
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	D3D12_RESOURCE_DESC resDesc = texBuff->GetDesc();

	srvDesc.Format = resDesc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2D Texture
	srvDesc.Texture2D.MipLevels = 1;

	device->CreateShaderResourceView(texBuff.Get(), // Buffer associated with view
		&srvDesc, // Texture setting information
		descHeapSRV->GetCPUDescriptorHandleForHeapStart() // Heap destination address
	);
}

void Model::Draw(ID3D12GraphicsCommandList* cmdList)
{
	// Set vertex buffer (VBV)
	cmdList->IASetVertexBuffers(0, 1, &vbView);

	// Set index buffer (IBV)
	cmdList->IASetIndexBuffer(&ibView);

	// Set descriptor heap
	ID3D12DescriptorHeap* ppHeaps[] = { descHeapSRV.Get() };
	cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// Shader Resource View set
	cmdList->SetGraphicsRootDescriptorTable(1, descHeapSRV->GetGPUDescriptorHandleForHeapStart());

	// Vertex Buffer View Set (Material)
	cmdList->SetGraphicsRootConstantBufferView(2, constBuffMaterial->GetGPUVirtualAddress());

	// Draw command
	cmdList->DrawIndexedInstanced((UINT)indices.size(), 1, 0, 0, 0);
}