#pragma once

#include "Renderer.h"
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcollision.h>
#include <vector>

struct MeshData {
    std::vector<DirectX::XMFLOAT4> vertices;
    UINT vertexCount = 0;
    UINT vertexStride = 32;
    std::vector<uint32_t> indices;
    UINT indexCount = 0;
};

MeshData createQuadMeshData();
MeshData createColoredQuadMeshData(const DirectX::XMFLOAT4& color);

MeshData createSphereMeshData(const DirectX::XMFLOAT4& color, float radius, int segments, int rings);
MeshData createBoxMeshData(const DirectX::XMFLOAT4& color, float width, float height, float depth);
MeshData createRingMeshData(const DirectX::XMFLOAT4& color, float innerRadius, float outerRadius, int segments);
MeshData createCircleLineMeshData(const DirectX::XMFLOAT4& color, int segments);

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