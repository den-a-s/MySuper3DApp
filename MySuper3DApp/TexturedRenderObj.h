#pragma once
#include "Renderer.h"
#include "ObjLoader.h"
#include <d3d11.h>
#include <directxmath.h>
#include <wrl.h>
#include <string>

struct TexturedRenderObj {
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

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mAlbedoSRV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mNormalSRV;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> mSamplerState;

    Microsoft::WRL::ComPtr<ID3DBlob> mVertexShaderByteCode;
    Microsoft::WRL::ComPtr<ID3DBlob> mPixelShaderByteCode;

    LoadedMaterial mMaterial;

    static TexturedRenderObj create(
        Renderer& r,
        const std::wstring& shaderFileName,
        const LoadedMesh& mesh,
        const LoadedMaterial& material,
        const std::string& textureBasePath
    );
};

struct alignas(16) ObjectConstantBuffer {
    DirectX::XMMATRIX world;
    DirectX::XMMATRIX view;
    DirectX::XMMATRIX projection;
    DirectX::XMFLOAT4 baseColor;
    DirectX::XMFLOAT4 ambientColor;
    DirectX::XMFLOAT4 lightDir;
    DirectX::XMFLOAT4 lightColor;
    DirectX::XMFLOAT4 cameraPos;
    float roughness;
    float metallic;
    float padding[2];
};

void drawTextured(const TexturedRenderObj& obj, const DirectX::XMMATRIX& world,
                   const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection,
                   const DirectX::XMFLOAT4& cameraPos, Renderer& r);
