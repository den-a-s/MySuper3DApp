#include "SquareRenderObj.h"
#include <windows.h>
#include <iostream>

SquareRenderObj SquareRenderObj::create(Renderer& r, const std::wstring& shaderFileName) {
    SquareRenderObj obj;

    ID3DBlob* errorVertexCode = nullptr;
    auto res = D3DCompileFromFile(
        shaderFileName.c_str(), nullptr, nullptr, "VSMain", "vs_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
        obj.mVertexShaderByteCode.GetAddressOf(), &errorVertexCode);
    if (FAILED(res)) {
        if (errorVertexCode) {
            char* compileErrors = (char*)(errorVertexCode->GetBufferPointer());
            std::cout << compileErrors << std::endl;
        } else {
            MessageBox(r.mDisplay->getHandlerWindow(), L"MyVeryFirstShader.hlsl",
                       L"Missing Shader File", MB_OK);
        }
        throw std::runtime_error("FAILED Compile shader");
    }

    D3D_SHADER_MACRO Shader_Macros[] = {
        "TCOLOR", "float4(0.0f, 1.0f, 0.0f, 1.0f)", nullptr, nullptr};
    ID3DBlob* errorPixelCode;
    res = D3DCompileFromFile(
        shaderFileName.c_str(), Shader_Macros, nullptr, "PSMain", "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
        obj.mPixelShaderByteCode.GetAddressOf(), &errorPixelCode);
    if (FAILED(res)) {
        if (errorPixelCode) {
            char* compileErrors = (char*)(errorPixelCode->GetBufferPointer());
            std::cout << compileErrors << std::endl;
        }
        throw std::runtime_error("FAILED Compile pixel shader");
    }

    r.mDevice->CreateVertexShader(obj.mVertexShaderByteCode->GetBufferPointer(),
                                  obj.mVertexShaderByteCode->GetBufferSize(),
                                  nullptr, obj.mVertexShader.GetAddressOf());
    r.mDevice->CreatePixelShader(obj.mPixelShaderByteCode->GetBufferPointer(),
                                 obj.mPixelShaderByteCode->GetBufferSize(),
                                 nullptr, obj.mPixelShader.GetAddressOf());

    D3D11_INPUT_ELEMENT_DESC inputElements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
         D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}};
    r.mDevice->CreateInputLayout(inputElements, 2,
                                 obj.mVertexShaderByteCode->GetBufferPointer(),
                                 obj.mVertexShaderByteCode->GetBufferSize(),
                                 obj.mInputLayout.GetAddressOf());

    CD3D11_RASTERIZER_DESC rastDesc = {};
    rastDesc.CullMode = D3D11_CULL_NONE;
    rastDesc.FillMode = D3D11_FILL_SOLID;
    r.mDevice->CreateRasterizerState(&rastDesc,
                                     obj.mRasterizerState.GetAddressOf());

    DirectX::XMFLOAT4 points[8] = {
        DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f),
        DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),
        DirectX::XMFLOAT4(-0.5f, -0.5f, 0.5f, 1.0f),
        DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),
        DirectX::XMFLOAT4(0.5f, -0.5f, 0.5f, 1.0f),
        DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f),
        DirectX::XMFLOAT4(-0.5f, 0.5f, 0.5f, 1.0f),
        DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
    };

    D3D11_BUFFER_DESC vertexBufDesc = {};
    vertexBufDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufDesc.ByteWidth = sizeof(DirectX::XMFLOAT4) * std::size(points);
    D3D11_SUBRESOURCE_DATA vertexData = {points};
    r.mDevice->CreateBuffer(&vertexBufDesc, &vertexData,
                            obj.mVertexBuffer.GetAddressOf());

    int indices[] = {0, 1, 2, 1, 0, 3};
    obj.mIndexCount = std::size(indices);
    D3D11_BUFFER_DESC indexBufDesc = {};
    indexBufDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexBufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufDesc.ByteWidth = sizeof(int) * obj.mIndexCount;
    D3D11_SUBRESOURCE_DATA indexData = {indices};
    r.mDevice->CreateBuffer(&indexBufDesc, &indexData,
                            obj.mIndexBuffer.GetAddressOf());

    obj.mStride = 32;
    obj.mOffset = 0;

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(DirectX::XMMATRIX) * 3;
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    r.mDevice->CreateBuffer(&cbDesc, nullptr,
                            obj.mConstantBuffer.GetAddressOf());

    return obj;
}

void draw(const SquareRenderObj& obj, const DirectX::XMMATRIX& world,
          const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection,
          Renderer& r) {
    struct ConstantBuffer {
        DirectX::XMMATRIX world;
        DirectX::XMMATRIX view;
        DirectX::XMMATRIX projection;
    } cb;
    cb.world = DirectX::XMMatrixTranspose(world);
    cb.view = DirectX::XMMatrixTranspose(view);
    cb.projection = DirectX::XMMatrixTranspose(projection);

    r.mContext->UpdateSubresource(obj.mConstantBuffer.Get(), 0, nullptr, &cb, 0,
                                  0);
    r.mContext->VSSetConstantBuffers(0, 1, obj.mConstantBuffer.GetAddressOf());

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

    r.mContext->DrawIndexed(obj.mIndexCount, 0, 0);
}