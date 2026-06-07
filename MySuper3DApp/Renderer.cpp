#include "Renderer.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <dxgi.h>
#include <windows.h>
#include <fstream>
#include <format>
#include <vector>
#include <json.hpp>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

Microsoft::WRL::ComPtr<IDXGIFactory> mustCreateDXGIFactory() {
    Microsoft::WRL::ComPtr<IDXGIFactory> pFactory1 = nullptr;
    HRESULT factoryRes = CreateDXGIFactory(IID_PPV_ARGS(&pFactory1));
    if (FAILED(factoryRes)) {
        throw std::runtime_error("FAILED CreateDXGIFactory");
    }
    return pFactory1;
}

std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter>> getAllDXGIAdapters() {
    auto pFactory1 = mustCreateDXGIFactory();
    std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter>> vAdapters;
    UINT i = 0;
    while (true) {
        Microsoft::WRL::ComPtr<IDXGIAdapter> pAdapter;
        HRESULT hr = pFactory1->EnumAdapters(i, pAdapter.GetAddressOf());
        if (hr == DXGI_ERROR_NOT_FOUND) break;
        if (SUCCEEDED(hr)) {
            vAdapters.push_back(std::move(pAdapter));
        } else {
            break;
        }
        ++i;
    }
    return vAdapters;
}

Microsoft::WRL::ComPtr<IDXGIAdapter> chooseAdapterFromConfig() {
    auto adapters = getAllDXGIAdapters();
    if (adapters.empty())
        throw std::runtime_error("No DXGI adapters found");

    nlohmann::json config;
    {
        std::ifstream f("adapters.json");
        if (!f.is_open())
            throw std::runtime_error("adapters.json not found in current working directory");
        f >> config;
    }

    auto& adapterCfg = config["adapter"];
    std::string mode = adapterCfg.value("mode", "auto");

    if (mode == "by_name") {
        std::string preferred = adapterCfg.value("preferred_name", "");
        for (auto& a : adapters) {
            DXGI_ADAPTER_DESC desc;
            if (SUCCEEDED(a->GetDesc(&desc))) {
                char name[128];
                wcstombs_s(nullptr, name, desc.Description, sizeof(name));
                if (strstr(name, preferred.c_str()))
                    return a;
            }
        }
    }

    if (mode == "by_index") {
        int idx = adapterCfg.value("index", 0);
        if (idx >= 0 && idx < (int)adapters.size())
            return adapters[idx];
    }

    SIZE_T bestMem = 0;
    size_t bestIdx = 0;
    for (size_t i = 0; i < adapters.size(); ++i) {
        DXGI_ADAPTER_DESC desc;
        if (SUCCEEDED(adapters[i]->GetDesc(&desc)) &&
            desc.DedicatedVideoMemory > bestMem) {
            bestMem = desc.DedicatedVideoMemory;
            bestIdx = i;
        }
    }
    return adapters[bestIdx];
}

Renderer Renderer::create() {
    Renderer renderer;
    renderer.mDisplay = std::make_shared<DisplayWin32>(L"MyFirst3DApp", 800, 800);
    renderer.mAdapter = chooseAdapterFromConfig();


    D3D_FEATURE_LEVEL featureLevel[] = {D3D_FEATURE_LEVEL_11_1};
    UINT featureLevelSize = std::size(featureLevel);

    DXGI_SWAP_CHAIN_DESC swapDesc = {
        .BufferDesc =
            {
                .Width = static_cast<UINT>(renderer.mDisplay->getScreenWidth()),
                .Height =
                    static_cast<UINT>(renderer.mDisplay->getScreenHeight()),
                .RefreshRate = {.Numerator = 60, .Denominator = 1},
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
                .Scaling = DXGI_MODE_SCALING_UNSPECIFIED,
            },
        .SampleDesc = {.Count = 1, .Quality = 0},
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = 2,
        .OutputWindow = renderer.mDisplay->getHandlerWindow(),
        .Windowed = true,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
        .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH};

    auto res = D3D11CreateDeviceAndSwapChain(
        renderer.mAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr,
        D3D11_CREATE_DEVICE_DEBUG, featureLevel, featureLevelSize,
        D3D11_SDK_VERSION, &swapDesc, renderer.mSwapChain.GetAddressOf(),
        renderer.mDevice.GetAddressOf(), nullptr,
        renderer.mContext.GetAddressOf());

    if (FAILED(res)) {
        throw std::runtime_error(
            std::format("FAILED D3D11CreateDeviceAndSwapChain code {}", res));
    }

    ID3D11Texture2D* backTex;
    res = renderer.mSwapChain->GetBuffer(0, IID_PPV_ARGS(&backTex));
    res = renderer.mDevice->CreateRenderTargetView(
        backTex, nullptr, renderer.mRTV.GetAddressOf());

    UINT width = renderer.mDisplay->getScreenWidth();
    UINT height = renderer.mDisplay->getScreenHeight();

    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* depthBuffer = nullptr;
    res = renderer.mDevice->CreateTexture2D(&depthDesc, nullptr, &depthBuffer);
    if (FAILED(res)) {
        throw std::runtime_error("FAILED CreateDepthStencilTexture");
    }
    res = renderer.mDevice->CreateDepthStencilView(
        depthBuffer, nullptr, renderer.mDepthStencilView.GetAddressOf());
    depthBuffer->Release();
    if (FAILED(res)) {
        throw std::runtime_error("FAILED CreateDepthStencilView");
    }

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    res = renderer.mDevice->CreateDepthStencilState(
        &dsDesc, renderer.mDepthStencilState.GetAddressOf());
    if (FAILED(res)) {
        throw std::runtime_error("FAILED CreateDepthStencilState");
    }

    backTex->Release();

    return renderer;
}

void Renderer::initCamera(float aspect) {
    mCamera.init(aspect);
}

void Renderer::beginFrame(float* clearColor) {
    mContext->ClearState();
    mContext->OMSetRenderTargets(1, mRTV.GetAddressOf(), mDepthStencilView.Get());
    mContext->ClearRenderTargetView(mRTV.Get(), clearColor);
    mContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    mContext->OMSetDepthStencilState(mDepthStencilState.Get(), 0);

    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(mDisplay->getScreenWidth());
    viewport.Height = static_cast<float>(mDisplay->getScreenHeight());
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.MinDepth = 0.0;
    viewport.MaxDepth = 1.0;

    mContext->RSSetViewports(1, &viewport);
}

void Renderer::endFrame() {
    ID3D11RenderTargetView* nullRTV = nullptr;
    mContext->OMSetRenderTargets(1, &nullRTV, nullptr);
    mSwapChain->Present(1, 0);
}

void Renderer::resize(int width, int height) {
    mContext->OMSetRenderTargets(0, nullptr, nullptr);
    mRTV.Reset();
    mDepthStencilView.Reset();

    HRESULT hr = mSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) {
        throw std::runtime_error(std::format("FAILED ResizeBuffers code {}", hr));
    }

    ID3D11Texture2D* backTex;
    hr = mSwapChain->GetBuffer(0, IID_PPV_ARGS(&backTex));
    if (FAILED(hr)) throw std::runtime_error("FAILED GetBuffer");
    hr = mDevice->CreateRenderTargetView(backTex, nullptr, mRTV.GetAddressOf());
    backTex->Release();
    if (FAILED(hr)) throw std::runtime_error("FAILED CreateRenderTargetView");

    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* depthBuffer = nullptr;
    hr = mDevice->CreateTexture2D(&depthDesc, nullptr, &depthBuffer);
    if (FAILED(hr)) throw std::runtime_error("FAILED CreateDepthStencilTexture");
    hr = mDevice->CreateDepthStencilView(depthBuffer, nullptr, mDepthStencilView.GetAddressOf());
    depthBuffer->Release();
    if (FAILED(hr)) throw std::runtime_error("FAILED CreateDepthStencilView");
}
