#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <dxgi.h>
#include <windows.h>
#include <wrl.h>

#include <chrono>
#include <format>
#include <iostream>
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam,
                         LPARAM lparam) {
  switch (umessage) {
    case WM_KEYDOWN: {
      // If a key is pressed send it to the input object so it can record that
      // state.
      std::cout << "Key: " << static_cast<unsigned int>(wparam) << std::endl;

      if (static_cast<unsigned int>(wparam) == 27) {
        PostQuitMessage(0);
      }
      return 0;
    }
    case WM_PAINT: {
      PAINTSTRUCT ps;
      BeginPaint(hwnd, &ps);
      EndPaint(hwnd, &ps);
      return 0;
    }
    default: {
      return DefWindowProc(hwnd, umessage, wparam, lparam);
    }
  }
}

Microsoft::WRL::ComPtr<IDXGIFactory1> mustCreateDXGIFactory1() {
  Microsoft::WRL::ComPtr<IDXGIFactory1> pFactory1 = nullptr;
  HRESULT factoryRes = CreateDXGIFactory1(IID_PPV_ARGS(&pFactory1));
  if (FAILED(factoryRes)) {
    throw std::runtime_error("FAILED CreateDXGIFactory1");
  }

  return pFactory1;
}

std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter>> getAllDXGIAdapters1() {
  auto pFactory1 = mustCreateDXGIFactory1();

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

Microsoft::WRL::ComPtr<IDXGIAdapter> chooseDXGIAdapters1() {
  auto adapters = getAllDXGIAdapters1();

  for (int i = 0; i < adapters.size(); ++i) {
    DXGI_ADAPTER_DESC adapterDesc;

    HRESULT hr = adapters[i]->GetDesc(&adapterDesc);
    if (FAILED(hr)) {
      break;
    }

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

class DisplayWin32 {
 public:
  DisplayWin32(const std::wstring& applicationName, const int screenHeight,
               const int screenWidth) {
    mApplicationName = applicationName;
    mHInstance = GetModuleHandle(nullptr);

    mWc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    mWc.lpfnWndProc = WndProc;
    mWc.cbClsExtra = 0;
    mWc.cbWndExtra = 0;
    mWc.hInstance = mHInstance;
    mWc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
    mWc.hIconSm = mWc.hIcon;
    mWc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    mWc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    mWc.lpszMenuName = nullptr;
    mWc.lpszClassName = mApplicationName.c_str();
    mWc.cbSize = sizeof(WNDCLASSEX);

    // Register the window class.
    RegisterClassEx(&mWc);

    mScreenHeight = screenHeight;
    mScreenWidth = screenWidth;

    RECT windowRect = {0, 0, screenWidth, screenHeight};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    auto dwStyle = WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_THICKFRAME;

    auto posX = (GetSystemMetrics(SM_CXSCREEN) - screenWidth) / 2;
    auto posY = (GetSystemMetrics(SM_CYSCREEN) - screenHeight) / 2;

    mHWnd = CreateWindowEx(WS_EX_APPWINDOW, mApplicationName.c_str(),
                           mApplicationName.c_str(), dwStyle, posX, posY,
                           windowRect.right - windowRect.left,
                           windowRect.bottom - windowRect.top, nullptr, nullptr,
                           mHInstance, nullptr);

    ShowWindow(mHWnd, SW_SHOW);
    SetForegroundWindow(mHWnd);
    SetFocus(mHWnd);

    ShowCursor(true);
  }

  HWND getHandlerWindow() { return mHWnd; }

  int getScreenHeight() { return mScreenHeight; }
  int getScreenWidth() { return mScreenWidth; }

 private:
  std::wstring mApplicationName;
  HINSTANCE mHInstance;
  WNDCLASSEX mWc;
  LONG mScreenWidth;
  LONG mScreenHeight;
  HWND mHWnd;
};

struct Transform {
  DirectX::XMFLOAT3 pos{0, 0, 0};
  DirectX::XMFLOAT3 rot{0, 0, 0};
  DirectX::XMFLOAT3 scale{1, 1, 1};
};

struct LogicObject {
  Transform prev;
  Transform curr;
  DirectX::XMFLOAT3 velocity{0, 0, 0};
};

struct Renderer {
  static Renderer create() {
    // Инициализация того куда мы будем всё писать и начальная настройка

    Renderer renderer;

    renderer.mAdapter = chooseDXGIAdapters1();

    D3D_FEATURE_LEVEL featureLevel[] = {D3D_FEATURE_LEVEL_11_1};
    UINT featureLevelSize = std::size(featureLevel);

    DXGI_SWAP_CHAIN_DESC swapDesc = {
        .BufferDesc =
            {
                .Width = static_cast<UINT>(renderer.mDisplay.getScreenWidth()),
                .Height =
                    static_cast<UINT>(renderer.mDisplay.getScreenHeight()),
                .RefreshRate =
                    {
                        .Numerator = 60,
                        .Denominator = 1,
                    },
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
                .Scaling = DXGI_MODE_SCALING_UNSPECIFIED,
            },
        .SampleDesc =
            {
                .Count = 1,
                .Quality = 0,
            },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = 2,
        .OutputWindow = renderer.mDisplay.getHandlerWindow(),
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
  
    return renderer;
  }

  void beginFrame(float* clearColor) {
    mContext->ClearState();
    mContext->OMSetRenderTargets(1, mRTV.GetAddressOf(), nullptr);
    mContext->ClearRenderTargetView(mRTV.Get(), clearColor);

    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(mDisplay.getScreenWidth());
    viewport.Height = static_cast<float>(mDisplay.getScreenHeight());
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.MinDepth = 0.0;
    viewport.MaxDepth = 1.0;

    mContext->RSSetViewports(1, &viewport);
  }

  void endFrame() { mSwapChain->Present(1, /*DXGI_PRESENT_DO_NOT_WAIT*/ 0); }

  DisplayWin32 mDisplay;
  Microsoft::WRL::ComPtr<IDXGIAdapter> mAdapter;
  Microsoft::WRL::ComPtr<ID3D11Device> mDevice;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> mContext;
  Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mRTV;

 private:
  Renderer() : mDisplay(DisplayWin32{L"My3DApp", 800, 800}) {}
};

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

  Microsoft::WRL::ComPtr<ID3DBlob> mVertexShaderByteCode;
  Microsoft::WRL::ComPtr<ID3DBlob> mPixelShaderByteCode;

  static SquareRenderObj create(Renderer& r,
                                const std::wstring& shaderFileName) {
    SquareRenderObj obj;
    // Создаём буферы для шейдеров
    ID3DBlob* errorVertexCode = nullptr;
    auto res = D3DCompileFromFile(
        /* L"./Shaders/MyVeryFirstShader.hlsl"*/ shaderFileName.c_str(),
        nullptr /*macros*/, nullptr /*include*/, "VSMain", "vs_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
        obj.mVertexShaderByteCode.GetAddressOf(), &errorVertexCode);

    if (FAILED(res)) {
      // If the shader failed to compile it should have written something to the
      // error message.
      if (errorVertexCode) {
        char* compileErrors = (char*)(errorVertexCode->GetBufferPointer());

        std::cout << compileErrors << std::endl;
      }
      // If there was  nothing in the error message then it simply could not
      // find the shader file itself.
      else {
        MessageBox(r.mDisplay.getHandlerWindow(), L"MyVeryFirstShader.hlsl",
                   L"Missing Shader File", MB_OK);
      }

      throw std::runtime_error("FAILED Compile shader");
    }

    D3D_SHADER_MACRO Shader_Macros[] = {
        "TCOLOR", "float4(0.0f, 1.0f, 0.0f, 1.0f)", nullptr, nullptr};

    ID3DBlob* errorPixelCode;
    res = D3DCompileFromFile(
        shaderFileName.c_str(), Shader_Macros /*macros*/, nullptr /*include*/,
        "PSMain", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
        obj.mPixelShaderByteCode.GetAddressOf(), &errorPixelCode);

    r.mDevice->CreateVertexShader(obj.mVertexShaderByteCode->GetBufferPointer(),
                                  obj.mVertexShaderByteCode->GetBufferSize(),
                                  nullptr, obj.mVertexShader.GetAddressOf());

    r.mDevice->CreatePixelShader(obj.mPixelShaderByteCode->GetBufferPointer(),
                                 obj.mPixelShaderByteCode->GetBufferSize(),
                                 nullptr, obj.mPixelShader.GetAddressOf());

    D3D11_INPUT_ELEMENT_DESC inputElements[] = {
        D3D11_INPUT_ELEMENT_DESC{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,
                                 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        D3D11_INPUT_ELEMENT_DESC{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
                                 D3D11_APPEND_ALIGNED_ELEMENT,
                                 D3D11_INPUT_PER_VERTEX_DATA, 0}};
    r.mDevice->CreateInputLayout(inputElements, 2,
                                 obj.mVertexShaderByteCode->GetBufferPointer(),
                                 obj.mVertexShaderByteCode->GetBufferSize(),
                                 obj.mInputLayout.GetAddressOf());

    CD3D11_RASTERIZER_DESC rastDesc = {};
    rastDesc.CullMode = D3D11_CULL_NONE;
    rastDesc.FillMode = D3D11_FILL_SOLID;

    res = r.mDevice->CreateRasterizerState(&rastDesc,
                                           obj.mRasterizerState.GetAddressOf());

    DirectX::XMFLOAT4 points[8] = {
        DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f),  // Точка
        DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),  // Цвет
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
    vertexBufDesc.CPUAccessFlags = 0;
    vertexBufDesc.MiscFlags = 0;
    vertexBufDesc.StructureByteStride = 0;
    vertexBufDesc.ByteWidth = sizeof(DirectX::XMFLOAT4) * std::size(points);

    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = points;
    vertexData.SysMemPitch = 0;
    vertexData.SysMemSlicePitch = 0;

    r.mDevice->CreateBuffer(&vertexBufDesc, &vertexData,
                            obj.mVertexBuffer.GetAddressOf());

    int indeces[] = {0, 1, 2, 1, 0, 3};
    obj.mIndexCount = std::size(indeces);
    D3D11_BUFFER_DESC indexBufDesc = {};
    indexBufDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexBufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufDesc.CPUAccessFlags = 0;
    indexBufDesc.MiscFlags = 0;
    indexBufDesc.StructureByteStride = 0;
    indexBufDesc.ByteWidth = sizeof(int) * obj.mIndexCount;

    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = indeces;
    indexData.SysMemPitch = 0;
    indexData.SysMemSlicePitch = 0;

    r.mDevice->CreateBuffer(&indexBufDesc, &indexData,
                            obj.mIndexBuffer.GetAddressOf());

    obj.mStride = 32;
    obj.mOffset = 0;

    return obj;
  }
};

void draw(const SquareRenderObj& obj, double alpha, Renderer& r) {
  r.mContext->RSSetState(obj.mRasterizerState.Get());

  r.mContext->IASetInputLayout(obj.mInputLayout.Get());
  r.mContext->IASetPrimitiveTopology(obj.topology);
  r.mContext->IASetIndexBuffer(obj.mIndexBuffer.Get(), DXGI_FORMAT_R32_UINT,
                               0);
  ID3D11Buffer* vbs[] = {obj.mVertexBuffer.Get()};
  UINT strides[] = {obj.mStride};
  UINT offsets[] = {obj.mOffset};

  r.mContext->IASetVertexBuffers(0, 1, vbs, strides, offsets);
  r.mContext->VSSetShader(obj.mVertexShader.Get(), nullptr, 0);
  r.mContext->PSSetShader(obj.mPixelShader.Get(), nullptr, 0);

  r.mContext->DrawIndexed(obj.mIndexCount, 0, 0);
}

struct TriangleRenderObj {
  Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexBuffer;
  UINT mStride = 0;
  UINT mOffset = 0;
  UINT mVertexCount = 0;
  D3D11_PRIMITIVE_TOPOLOGY mTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

  Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader;
  Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout;
  Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizerState;

  Microsoft::WRL::ComPtr<ID3DBlob> mVertexShaderByteCode;
  Microsoft::WRL::ComPtr<ID3DBlob> mPixelShaderByteCode;

  static TriangleRenderObj create(Renderer& r,
                                const std::wstring& shaderFileName) {
    TriangleRenderObj obj;
    // Создаём буферы для шейдеров
    ID3DBlob* errorVertexCode = nullptr;
    auto res = D3DCompileFromFile(
        /* L"./Shaders/MyVeryFirstShader.hlsl"*/ shaderFileName.c_str(),
        nullptr /*macros*/, nullptr /*include*/, "VSMain", "vs_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
        obj.mVertexShaderByteCode.GetAddressOf(), &errorVertexCode);

    if (FAILED(res)) {
      // If the shader failed to compile it should have written something to the
      // error message.
      if (errorVertexCode) {
        char* compileErrors = (char*)(errorVertexCode->GetBufferPointer());

        std::cout << compileErrors << std::endl;
      }
      // If there was  nothing in the error message then it simply could not
      // find the shader file itself.
      else {
        MessageBox(r.mDisplay.getHandlerWindow(), L"MyVeryFirstShader.hlsl",
                   L"Missing Shader File", MB_OK);
      }

      throw std::runtime_error("FAILED Compile shader");
    }

    D3D_SHADER_MACRO Shader_Macros[] = {
        "TCOLOR", "float4(0.0f, 1.0f, 0.0f, 1.0f)", nullptr, nullptr};

    ID3DBlob* errorPixelCode;
    res = D3DCompileFromFile(
        shaderFileName.c_str(), Shader_Macros /*macros*/, nullptr /*include*/,
        "PSMain", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
        obj.mPixelShaderByteCode.GetAddressOf(), &errorPixelCode);

    r.mDevice->CreateVertexShader(obj.mVertexShaderByteCode->GetBufferPointer(),
                                  obj.mVertexShaderByteCode->GetBufferSize(),
                                  nullptr, obj.mVertexShader.GetAddressOf());

    r.mDevice->CreatePixelShader(obj.mPixelShaderByteCode->GetBufferPointer(),
                                 obj.mPixelShaderByteCode->GetBufferSize(),
                                 nullptr, obj.mPixelShader.GetAddressOf());

    D3D11_INPUT_ELEMENT_DESC inputElements[] = {
        D3D11_INPUT_ELEMENT_DESC{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,
                                 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        D3D11_INPUT_ELEMENT_DESC{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
                                 D3D11_APPEND_ALIGNED_ELEMENT,
                                 D3D11_INPUT_PER_VERTEX_DATA, 0}};
    r.mDevice->CreateInputLayout(inputElements, 2,
                                 obj.mVertexShaderByteCode->GetBufferPointer(),
                                 obj.mVertexShaderByteCode->GetBufferSize(),
                                 obj.mInputLayout.GetAddressOf());

    CD3D11_RASTERIZER_DESC rastDesc = {};
    rastDesc.CullMode = D3D11_CULL_NONE;
    rastDesc.FillMode = D3D11_FILL_SOLID;

    res = r.mDevice->CreateRasterizerState(&rastDesc,
                                           obj.mRasterizerState.GetAddressOf());

    auto color = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

    DirectX::XMFLOAT4 points[] = {
        DirectX::XMFLOAT4(1.0f, 0.9f, 0.5f, 1.0f),  // Точка
        color,                                      // Цвет
        DirectX::XMFLOAT4(-0.5f, 1.0f, 0.5f, 1.0f),
        color,
        DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f),
        color,
    };

    obj.mVertexCount = std::size(points) / 2;

    D3D11_BUFFER_DESC vertexBufDesc = {};
    vertexBufDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufDesc.CPUAccessFlags = 0;
    vertexBufDesc.MiscFlags = 0;
    vertexBufDesc.StructureByteStride = 0;
    vertexBufDesc.ByteWidth = sizeof(DirectX::XMFLOAT4) * std::size(points);

    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = points;
    vertexData.SysMemPitch = 0;
    vertexData.SysMemSlicePitch = 0;

    r.mDevice->CreateBuffer(&vertexBufDesc, &vertexData,
                            obj.mVertexBuffer.GetAddressOf());

    obj.mStride = sizeof(DirectX::XMFLOAT4) * 2;
    obj.mOffset = 0;

    return obj;
  }
};

void draw(const TriangleRenderObj& obj, double alpha, Renderer& r) {
  r.mContext->RSSetState(obj.mRasterizerState.Get());

  r.mContext->IASetInputLayout(obj.mInputLayout.Get());
  r.mContext->IASetPrimitiveTopology(obj.mTopology);
  ID3D11Buffer* vbs[] = {obj.mVertexBuffer.Get()};
  UINT strides[] = {obj.mStride};
  UINT offsets[] = {obj.mOffset};

  r.mContext->IASetVertexBuffers(0, 1, vbs, strides, offsets);
  r.mContext->VSSetShader(obj.mVertexShader.Get(), nullptr, 0);
  r.mContext->PSSetShader(obj.mPixelShader.Get(), nullptr, 0);

  r.mContext->Draw(obj.mVertexCount, 0);
}

class Renderable {
  struct IRenderable {
    virtual ~IRenderable() = default;
    virtual std::unique_ptr<IRenderable> copy_() const = 0;
    virtual void draw_(double delta, Renderer& r) const = 0;
  };

  template <typename T>
  struct RenderableObject final : IRenderable {
    T data_;
    RenderableObject(T x) : data_(std::move(x)) {}
    std::unique_ptr<IRenderable> copy_() const override {
      return std::make_unique<RenderableObject>(*this);
    }
    void draw_(double delta, Renderer& r) const override;
  };

  std::unique_ptr<IRenderable> self_;

 public:
  template <typename T>
  Renderable(T x)
      : self_(std::make_unique<RenderableObject<T>>(std::move(x))) {}

  // copy ctor, move ctor and assignment
  Renderable(const Renderable& x) : self_(x.self_->copy_()) {}
  Renderable(Renderable&& x) noexcept = default;
  Renderable& operator=(Renderable x) noexcept {
    self_ = std::move(x.self_);
    return *this;
  }

  friend void draw(const Renderable& x, double delta, Renderer& r) {
    x.self_->draw_(delta, r);
  }
};

template <typename T>
void Renderable::RenderableObject<T>::draw_(double delta, Renderer& r) const {
  ::draw(data_, delta, r);
}

class Game {
 public:
  Game() : mRenderer(Renderer::create()) { init(); }

  void init() {
    mRenderObjects.push_back(SquareRenderObj::create(
        mRenderer, L"./Shaders/MyVeryFirstShader.hlsl"));

    mRenderObjects.push_back(TriangleRenderObj::create(
        mRenderer, L"./Shaders/MyVeryFirstShader.hlsl"));
  }

  void run() {
    std::chrono::time_point<std::chrono::steady_clock> PrevTime =
        std::chrono::steady_clock::now();
    unsigned int frameCount = 0;

    while (!mIsExitRequested) {
      
      handleInput();

      auto curTime = std::chrono::steady_clock::now();
      float deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(
                            curTime - PrevTime)
                            .count() /
                        1000000.0f;
      PrevTime = curTime;

      mTotalTime += deltaTime;
      frameCount++;

      if (mTotalTime > 1.0f) {
        float fps = frameCount / mTotalTime;

        mTotalTime -= 1.0f;

         WCHAR text[256];
         swprintf_s(text, TEXT("FPS: %f"), fps);
         SetWindowText(mRenderer.mDisplay.getHandlerWindow(), text);

        frameCount = 0;
      }
      Render(deltaTime);
    }

    std::cout << "Hello World!\n";
  }

 private:
  void handleInput() {
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    if (msg.message == WM_QUIT) {
      mIsExitRequested = true;
    }
  }

  void Render(double alpha) {
    float color[]{0.0, 0.0, 0.0, 1.0};
    mRenderer.beginFrame(color);

    for (const auto& obj : mRenderObjects) {
      // TODO Добавить объекты игровой логики
      draw(obj, alpha, mRenderer);
    }

    mRenderer.endFrame();
  }

  float mTotalTime;
  bool mIsExitRequested;

  Renderer mRenderer;

  std::vector<Renderable> mRenderObjects;
};

int main() {
  Game game;
  game.run();
}
