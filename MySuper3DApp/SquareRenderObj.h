#pragma once

#include "Renderer.h"
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcollision.h>

struct SquareRenderObj {
    Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> mIndexBuffer;
    UINT mStride = 0;
    UINT mOffset = 0;
    UINT mIndexCount = 0;
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizerState;
    Microsoft::WRL::ComPtr<ID3D11Buffer> mConstantBuffer;

    Microsoft::WRL::ComPtr<ID3DBlob> mVertexShaderByteCode;
    Microsoft::WRL::ComPtr<ID3DBlob> mPixelShaderByteCode;

    static SquareRenderObj create(Renderer& r, const std::wstring& shaderFileName);
};

void draw(const SquareRenderObj& obj, const DirectX::XMMATRIX& world,
          const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection,
          Renderer& r);