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
    /// Issuing drawing command
    /// </summary>
    void Draw(ID3D12GraphicsCommandList* cmdList);
};