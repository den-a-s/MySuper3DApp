#pragma once

#include "Renderer.h"
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcollision.h>

struct MeshData {
    const void* vertices = nullptr;
    UINT vertexCount = 0;
    UINT vertexStride = 0;
    const uint32_t* indices = nullptr;
    UINT indexCount = 0;
};

MeshData createQuadMeshData();
MeshData createColoredQuadMeshData(const DirectX::XMFLOAT4& color);

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

    static SquareRenderObj create(Renderer& r, const std::wstring& shaderFileName, const MeshData& meshData);
};

void draw(const SquareRenderObj& obj, const DirectX::XMMATRIX& world,
          const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection,
          Renderer& r);

void draw(const SquareRenderObj& obj, const DirectX::XMFLOAT3& position,
          const DirectX::XMFLOAT3& scale, Renderer& r);