#include "Renderer.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <dxgi.h>
#include <windows.h>
#include <iostream>
#include <format>
#include <vector>

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

Microsoft::WRL::ComPtr<IDXGIAdapter> chooseDXGIAdapters() {
    auto adapters = getAllDXGIAdapters();
    for (int i = 0; i < adapters.size(); ++i) {
        DXGI_ADAPTER_DESC adapterDesc;
        HRESULT hr = adapters[i]->GetDesc(&adapterDesc);
        if (FAILED(hr)) break;
        char cardName[128];
        size_t convertedChars;
        wcstombs_s(&convertedChars, cardName, adapterDesc.Description,
                   sizeof(cardName));
        std::cout << std::format("Video adapter{}: {} \n", i, cardName);
    }
    int numAdapter;
    std::cout << "Choose adapter:";
    std::cin >> numAdapter;
    if (0 <= numAdapter && numAdapter < adapters.size()) {
        return adapters[numAdapter];
    } else {
        throw std::runtime_error("FAILED Choose not exists adapter");
    }
}

Renderer Renderer::create() {
    Renderer renderer;
    renderer.mDisplay = std::make_shared<DisplayWin32>(L"Pong Game", 800, 800);
    renderer.mAdapter = chooseDXGIAdapters();


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
    backTex->Release();

    return renderer;
}

void Renderer::beginFrame(float* clearColor) {
    mContext->ClearState();
    mContext->OMSetRenderTargets(1, mRTV.GetAddressOf(), nullptr);
    mContext->ClearRenderTargetView(mRTV.Get(), clearColor);

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
    mSwapChain->Present(1, 0); 
}