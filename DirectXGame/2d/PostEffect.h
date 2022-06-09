#pragma once
#include "Sprite.h"

class PostEffect :
    public Sprite
{
public:
    /// <summary>
    /// Constructor
    /// </summary>
    PostEffect();

    /// <summary>
    /// Initialization
    /// </summary>
    void Initialize();

    /// <summary>
    /// PreDraw Scene
    /// </summary>
    void PreDrawScene(ID3D12GraphicsCommandList* cmdList);

    /// <summary>
    /// Issuing drawing command
    /// </summary>
    void Draw(ID3D12GraphicsCommandList* cmdList);

    /// <summary>
    /// PostDraw Scene
    /// </summary>
    void PostDrawScene(ID3D12GraphicsCommandList* cmdList);

public: // Member Variables
    // Texture Buffer
    ComPtr<ID3D12Resource> texBuff;

    // SRV descript heap
    ComPtr<ID3D12DescriptorHeap> descHeapSRV;

    // Depth Buffer
    ComPtr<ID3D12Resource> depthBuff;

    // RTV descriptor Heap
    ComPtr<ID3D12DescriptorHeap> descHeapRTV;

    // DSV descriptor heap
    ComPtr<ID3D12DescriptorHeap> descHeapDSV;

    // Screen clear color
    static const float clearColor[4];
};