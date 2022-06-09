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
    /// Issuing drawing command
    /// </summary>
    void Draw(ID3D12GraphicsCommandList* cmdList);

public: // Member Variables
    // Texture Buffer
    ComPtr<ID3D12Resource> texBuff;

    // SRV descript heap
    ComPtr<ID3D12DescriptorHeap> descHeapSRV;
};