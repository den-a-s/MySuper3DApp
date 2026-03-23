#pragma once

#include "DisplayWin32.h"
#include <d3d11.h>
#include <dxgi.h>
#include <wrl.h>
#include <vector>
#include <memory>
#include <windows.h>

class Renderer {
public:
    static Renderer create();
    
    void beginFrame(float* clearColor);
    void endFrame();

    std::shared_ptr<DisplayWin32> mDisplay;
    Microsoft::WRL::ComPtr<IDXGIAdapter> mAdapter;
    Microsoft::WRL::ComPtr<ID3D11Device> mDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> mContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mRTV;

private:
    Renderer() = default;
};

Microsoft::WRL::ComPtr<IDXGIFactory> mustCreateDXGIFactory();
std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter>> getAllDXGIAdapters();
Microsoft::WRL::ComPtr<IDXGIAdapter> chooseDXGIAdapters();