#include "TexturedRenderObj.h"
#include "DirectXTex/DDSTextureLoader11.h"
#include <d3dcompiler.h>
#include <windows.h>
#include <stdexcept>
#include <format>

static std::wstring toWide(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], len);
    return w;
}

static Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> loadTextureFromFile(
    ID3D11Device* device, const std::string& filepath)
{
    if (filepath.empty()) return nullptr;

    std::wstring wpath = toWide(filepath);

    Microsoft::WRL::ComPtr<ID3D11Resource> texture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    HRESULT hr = DirectX::CreateDDSTextureFromFile(
        device, wpath.c_str(),
        texture.GetAddressOf(),
        srv.GetAddressOf());

    if (FAILED(hr)) {
        OutputDebugStringA(("Failed to load DDS texture: " + filepath + "\n").c_str());
        return nullptr;
    }

    return srv;
}

TexturedRenderObj TexturedRenderObj::create(
    Renderer& r, const std::wstring& shaderFileName,
    const LoadedMesh& mesh, const LoadedMaterial& material,
    const std::string& textureBasePath)
{
    TexturedRenderObj obj;
    obj.mMaterial = material;

    ID3DBlob* errorVertexCode = nullptr;
    auto res = D3DCompileFromFile(
        shaderFileName.c_str(), nullptr, nullptr, "VSMain", "vs_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
        obj.mVertexShaderByteCode.GetAddressOf(), &errorVertexCode);
    if (FAILED(res)) {
        if (errorVertexCode) {
            char* compileErrors = (char*)(errorVertexCode->GetBufferPointer());
            OutputDebugStringA(compileErrors);
            OutputDebugStringA("\n");
        }
        throw std::runtime_error("FAILED Compile vertex shader for ObjectShader");
    }

    ID3DBlob* errorPixelCode = nullptr;
    res = D3DCompileFromFile(
        shaderFileName.c_str(), nullptr, nullptr, "PSMain", "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
        obj.mPixelShaderByteCode.GetAddressOf(), &errorPixelCode);
    if (FAILED(res)) {
        if (errorPixelCode) {
            char* compileErrors = (char*)(errorPixelCode->GetBufferPointer());
            OutputDebugStringA(compileErrors);
            OutputDebugStringA("\n");
        }
        throw std::runtime_error("FAILED Compile pixel shader for ObjectShader");
    }

    if (FAILED(r.mDevice->CreateVertexShader(
            obj.mVertexShaderByteCode->GetBufferPointer(),
            obj.mVertexShaderByteCode->GetBufferSize(), nullptr,
            obj.mVertexShader.GetAddressOf()))) {
        throw std::runtime_error("FAILED CreateVertexShader");
    }
    if (FAILED(r.mDevice->CreatePixelShader(
            obj.mPixelShaderByteCode->GetBufferPointer(),
            obj.mPixelShaderByteCode->GetBufferSize(), nullptr,
            obj.mPixelShader.GetAddressOf()))) {
        throw std::runtime_error("FAILED CreatePixelShader");
    }

    D3D11_INPUT_ELEMENT_DESC inputElements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    if (FAILED(r.mDevice->CreateInputLayout(
            inputElements, 3, obj.mVertexShaderByteCode->GetBufferPointer(),
            obj.mVertexShaderByteCode->GetBufferSize(),
            obj.mInputLayout.GetAddressOf()))) {
        throw std::runtime_error("FAILED CreateInputLayout");
    }

    CD3D11_RASTERIZER_DESC rastDesc = {};
    rastDesc.CullMode = D3D11_CULL_BACK;
    rastDesc.FillMode = D3D11_FILL_SOLID;
    if (FAILED(r.mDevice->CreateRasterizerState(
            &rastDesc, obj.mRasterizerState.GetAddressOf()))) {
        throw std::runtime_error("FAILED CreateRasterizerState");
    }

    UINT vertexStride = sizeof(VertexPNT);
    D3D11_BUFFER_DESC vertexBufDesc = {};
    vertexBufDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufDesc.ByteWidth = vertexStride * (UINT)mesh.vertices.size();
    D3D11_SUBRESOURCE_DATA vertexData = {mesh.vertices.data()};
    if (FAILED(r.mDevice->CreateBuffer(&vertexBufDesc, &vertexData,
                                        obj.mVertexBuffer.GetAddressOf()))) {
        throw std::runtime_error("FAILED CreateVertexBuffer");
    }

    obj.mIndexCount = (UINT)mesh.indices.size();
    D3D11_BUFFER_DESC indexBufDesc = {};
    indexBufDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexBufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufDesc.ByteWidth = sizeof(uint32_t) * obj.mIndexCount;
    D3D11_SUBRESOURCE_DATA indexData = {mesh.indices.data()};
    if (FAILED(r.mDevice->CreateBuffer(&indexBufDesc, &indexData,
                                        obj.mIndexBuffer.GetAddressOf()))) {
        throw std::runtime_error("FAILED CreateIndexBuffer");
    }

    obj.mStride = vertexStride;
    obj.mOffset = 0;

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(ObjectConstantBuffer);
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(r.mDevice->CreateBuffer(&cbDesc, nullptr,
                                        obj.mConstantBuffer.GetAddressOf()))) {
        throw std::runtime_error("FAILED CreateConstantBuffer");
    }

    std::string base = textureBasePath.empty() ? "" : textureBasePath;
    if (!base.empty() && base.back() != '/' && base.back() != '\\')
        base += '/';

    if (!material.albedoPath.empty()) {
        obj.mAlbedoSRV = loadTextureFromFile(r.mDevice.Get(), material.albedoPath);
        if (!obj.mAlbedoSRV) {
            std::string fallback = base + material.albedoPath;
            obj.mAlbedoSRV = loadTextureFromFile(r.mDevice.Get(), fallback);
        }
    }
    if (!obj.mAlbedoSRV)
        obj.mAlbedoSRV = r.mWhiteTextureSRV;

    if (!material.normalPath.empty()) {
        obj.mNormalSRV = loadTextureFromFile(r.mDevice.Get(), material.normalPath);
        if (!obj.mNormalSRV) {
            std::string fallback = base + material.normalPath;
            obj.mNormalSRV = loadTextureFromFile(r.mDevice.Get(), fallback);
        }
    }
    if (!obj.mNormalSRV)
        obj.mNormalSRV = r.mFlatNormalSRV;

    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(r.mDevice->CreateSamplerState(&sampDesc,
                                              obj.mSamplerState.GetAddressOf()))) {
        throw std::runtime_error("FAILED CreateSamplerState");
    }

    return obj;
}

void drawTextured(const TexturedRenderObj& obj, const DirectX::XMMATRIX& world,
                   const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection,
                   const DirectX::XMFLOAT4& cameraPos, Renderer& r)
{
    ObjectConstantBuffer cb;
    cb.world = DirectX::XMMatrixTranspose(world);
    cb.view = DirectX::XMMatrixTranspose(view);
    cb.projection = DirectX::XMMatrixTranspose(projection);
    cb.baseColor = obj.mMaterial.baseColor;
    cb.ambientColor = obj.mMaterial.ambientColor;
    cb.lightDir = DirectX::XMFLOAT4(0.0f, -0.5f, 0.7f, 0.0f);
    cb.lightColor = DirectX::XMFLOAT4(1.0f, 0.95f, 0.85f, 1.0f);
    cb.cameraPos = cameraPos;
    cb.roughness = obj.mMaterial.specularExponent / 128.0f;
    cb.metallic = 0.0f;
    cb.specularColor = obj.mMaterial.specularColor;
    if (cb.specularColor.x == 0.0f && cb.specularColor.y == 0.0f && cb.specularColor.z == 0.0f)
        cb.specularColor = {0.5f, 0.5f, 0.5f, 1.0f};

    r.mContext->UpdateSubresource(obj.mConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
    r.mContext->VSSetConstantBuffers(0, 1, obj.mConstantBuffer.GetAddressOf());
    r.mContext->PSSetConstantBuffers(0, 1, obj.mConstantBuffer.GetAddressOf());
    if (r.mLightBuffer)
        r.mContext->PSSetConstantBuffers(1, 1, r.mLightBuffer.GetAddressOf());

    r.mContext->RSSetState(obj.mRasterizerState.Get());
    r.mContext->IASetInputLayout(obj.mInputLayout.Get());
    r.mContext->IASetPrimitiveTopology(obj.topology);
    r.mContext->IASetIndexBuffer(obj.mIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    ID3D11Buffer* vbs[] = {obj.mVertexBuffer.Get()};
    UINT strides[] = {obj.mStride};
    UINT offsets[] = {obj.mOffset};
    r.mContext->IASetVertexBuffers(0, 1, vbs, strides, offsets);
    r.mContext->VSSetShader(obj.mVertexShader.Get(), nullptr, 0);
    r.mContext->PSSetShader(obj.mPixelShader.Get(), nullptr, 0);

    ID3D11ShaderResourceView* srvs[] = {
        obj.mAlbedoSRV.Get(),
        obj.mNormalSRV.Get()
    };
    r.mContext->PSSetShaderResources(0, 2, srvs);
    r.mContext->PSSetSamplers(0, 1, obj.mSamplerState.GetAddressOf());

    r.mContext->DrawIndexed(obj.mIndexCount, 0, 0);
}
