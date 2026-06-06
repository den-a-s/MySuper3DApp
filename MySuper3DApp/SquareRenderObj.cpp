#include "SquareRenderObj.h"
#include <windows.h>
#include <stdexcept>


MeshData createQuadMeshData() {
    static DirectX::XMFLOAT4 s_points[8] = {
        DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f),
        DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),
        DirectX::XMFLOAT4(-0.5f, -0.5f, 0.5f, 1.0f),
        DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),
        DirectX::XMFLOAT4(0.5f, -0.5f, 0.5f, 1.0f),
        DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f),
        DirectX::XMFLOAT4(-0.5f, 0.5f, 0.5f, 1.0f),
        DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
    };
    static uint32_t s_indices[6] = {0, 1, 2, 1, 0, 3};

    MeshData md;
    md.vertices = s_points;
    md.vertexCount = 4;
    md.vertexStride = 32;
    md.indices = s_indices;
    md.indexCount = 6;
    return md;
}


SquareRenderObj SquareRenderObj::create(Renderer& r, const std::wstring& shaderFileName, const MeshData& meshData) {
    SquareRenderObj obj;

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
            OutputDebugStringA(compileErrors);
            OutputDebugStringA("\n");
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

    D3D11_BUFFER_DESC vertexBufDesc = {};
    vertexBufDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufDesc.ByteWidth = meshData.vertexStride * meshData.vertexCount;
    D3D11_SUBRESOURCE_DATA vertexData = {meshData.vertices};
    r.mDevice->CreateBuffer(&vertexBufDesc, &vertexData,
                            obj.mVertexBuffer.GetAddressOf());

    obj.mIndexCount = meshData.indexCount;
    D3D11_BUFFER_DESC indexBufDesc = {};
    indexBufDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexBufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufDesc.ByteWidth = sizeof(uint32_t) * obj.mIndexCount;
    D3D11_SUBRESOURCE_DATA indexData = {meshData.indices};
    r.mDevice->CreateBuffer(&indexBufDesc, &indexData,
                            obj.mIndexBuffer.GetAddressOf());

    obj.mStride = meshData.vertexStride;
    obj.mOffset = 0;

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(DirectX::XMMATRIX) * 3;
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    r.mDevice->CreateBuffer(&cbDesc, nullptr,
                            obj.mConstantBuffer.GetAddressOf());

    return obj;
}

void draw(const SquareRenderObj& obj, const DirectX::XMFLOAT3& position,
          const DirectX::XMFLOAT3& scale, Renderer& r) {
    DirectX::XMMATRIX world =
        DirectX::XMMatrixScaling(scale.x, scale.y, scale.z) *
        DirectX::XMMatrixTranslation(position.x, position.y, position.z);
    auto& camera = r.getCamera();
    draw(obj, world, camera.getView(), camera.getProjection(), r);
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