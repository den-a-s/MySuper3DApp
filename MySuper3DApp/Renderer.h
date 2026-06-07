#pragma once

#include "Camera.h"
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
    void resize(int width, int height);

    void initCamera(float aspect);

    Camera& getCamera() { return mCamera; }

    std::shared_ptr<DisplayWin32> mDisplay;
    Microsoft::WRL::ComPtr<IDXGIAdapter> mAdapter;
    Microsoft::WRL::ComPtr<ID3D11Device> mDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> mContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mRTV;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> mDepthStencilBuffer;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mDepthStencilView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthStencilState;

private:
    Renderer() = default;

    Camera mCamera;
};

Microsoft::WRL::ComPtr<IDXGIFactory> mustCreateDXGIFactory();
std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter>> getAllDXGIAdapters();
Microsoft::WRL::ComPtr<IDXGIAdapter> chooseAdapterFromConfig();
