#include "Game.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <windows.h>
#include <cstdio>
#include <algorithm>
#include <fstream>
#include <json.hpp>

Game::Game() : mRenderer(Renderer::create()) {
    init();
}

void Game::run() {
    std::chrono::time_point<std::chrono::steady_clock> prevTime =
        std::chrono::steady_clock::now();

    while (!mIsExitRequested) {
        auto curTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(
                              curTime - prevTime)
                              .count() /
                          1000000.0f;
        prevTime = curTime;

        handleInput(deltaTime);
        Update(deltaTime);
        Render(deltaTime);
    }
}

void Game::init() {
    auto sunColor = DirectX::XMFLOAT4(1.0f, 0.85f, 0.3f, 1.0f);
    auto sunMesh = createSphereMeshData(sunColor, 0.6f, 16, 12);
    mSunObj = SquareRenderObj::create(mRenderer, L"../Shaders/MyVeryFirstShader.hlsl", sunMesh);

    reloadPlanets();

    mRenderer.initCamera(1.0f);

    SetWindowText(mRenderer.mDisplay->getHandlerWindow(), L"Planet Simulation");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(mRenderer.mDisplay->getHandlerWindow());
    ImGui_ImplDX11_Init(mRenderer.mDevice.Get(), mRenderer.mContext.Get());
}

void Game::reloadPlanets() {
    using json = nlohmann::json;

    std::ifstream f(mPlanetsJsonPath);
    if (!f.is_open()) {
        mReloadStatus = "planets.json not found, keeping current planets";
        return;
    }

    json j;
    try {
        f >> j;
    } catch (...) {
        mReloadStatus = "Failed to parse planets.json, keeping current planets";
        return;
    }

    auto white = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);

    mPlanets.clear();

    for (auto& item : j["planets"]) {
        PlanetBody body;

        body.data.orbitRadius = item["orbitRadius"];
        body.data.orbitSpeed = item["orbitSpeed"];
        body.data.angle = 0.0f;
        body.data.rotationSpeed = item["rotationSpeed"];
        body.data.rotationAngle = 0.0f;
        body.data.size = item["size"];
        {
            auto& c = item["color"];
            body.data.color = DirectX::XMFLOAT4(c[0], c[1], c[2], c[3]);
        }

        body.planetIsBox = item["planetIsBox"];

        if (body.planetIsBox) {
            auto mesh = createBoxMeshData(body.data.color, body.data.size * 2, body.data.size * 2, body.data.size * 2);
            body.planetObj = SquareRenderObj::create(mRenderer, L"../Shaders/MyVeryFirstShader.hlsl", mesh);
        } else {
            auto mesh = createSphereMeshData(body.data.color, 1.0f, 16, 12);
            body.planetObj = SquareRenderObj::create(mRenderer, L"../Shaders/MyVeryFirstShader.hlsl", mesh);
        }

        auto& moon = item["moon"];
        body.data.moon.orbitRadius = moon["orbitRadius"];
        body.data.moon.orbitSpeed = moon["orbitSpeed"];
        body.data.moon.angle = 0.0f;
        body.data.moon.rotationSpeed = moon["rotationSpeed"];
        body.data.moon.rotationAngle = 0.0f;
        body.data.moon.size = moon["size"];
        {
            auto& mc = moon["color"];
            body.data.moon.color = DirectX::XMFLOAT4(mc[0], mc[1], mc[2], mc[3]);
        }

        body.moonIsBox = item["moonIsBox"];

        if (body.moonIsBox) {
            auto mesh = createBoxMeshData(body.data.moon.color, body.data.moon.size * 2, body.data.moon.size * 2, body.data.moon.size * 2);
            body.moonObj = SquareRenderObj::create(mRenderer, L"../Shaders/MyVeryFirstShader.hlsl", mesh);
        } else {
            auto mesh = createSphereMeshData(body.data.moon.color, 1.0f, 12, 8);
            body.moonObj = SquareRenderObj::create(mRenderer, L"../Shaders/MyVeryFirstShader.hlsl", mesh);
        }

        auto ringMesh = createRingMeshData(white, 0.98f, 1.02f, 48);
        body.orbitObj = SquareRenderObj::create(mRenderer, L"../Shaders/MyVeryFirstShader.hlsl", ringMesh);

        mPlanets.push_back(std::move(body));
    }

    mReloadStatus = "Reloaded " + std::to_string(mPlanets.size()) + " planets";
}

Game::~Game() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Game::handleInput(float deltaTime) {
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        switch (msg.message) {
        case WM_MOUSEMOVE:
            if (mMousePressed) {
                int x = static_cast<int>(LOWORD(msg.lParam));
                int y = static_cast<int>(HIWORD(msg.lParam));
                float dx = static_cast<float>(x - mMouseX) * 0.005f;
                float dy = static_cast<float>(mMouseY - y) * 0.005f;
                auto& cam = mRenderer.getCamera();
                if (cam.mMode == CameraMode::FPS) {
                    cam.rotateFPS(dx, dy);
                } else {
                    cam.rotateOrbit(dx, dy);
                }
                mMouseX = x;
                mMouseY = y;
            }
            break;
        case WM_LBUTTONDOWN:
            if (ImGui::GetIO().WantCaptureMouse)
                break;
            mMousePressed = true;
            mMouseX = static_cast<int>(LOWORD(msg.lParam));
            mMouseY = static_cast<int>(HIWORD(msg.lParam));
            break;
        case WM_LBUTTONUP:
            if (ImGui::GetIO().WantCaptureMouse)
                break;
            mMousePressed = false;
            break;
        case WM_QUIT:
            mIsExitRequested = true;
            return;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (mInputManager.isExit()) {
        mIsExitRequested = true;
        return;
    }

    if (mInputManager.isSwitchCamera()) {
        if (!mCamSwitchHeld) {
            mCamSwitchHeld = true;
            auto& cam = mRenderer.getCamera();
            if (cam.mMode == CameraMode::FPS) {
                cam.setMode(CameraMode::Orbit);
            } else {
                cam.setMode(CameraMode::FPS);
            }
            cam.update();
        }
    } else {
        mCamSwitchHeld = false;
    }

    float moveSpeed = 6.0f * deltaTime;
    auto& cam = mRenderer.getCamera();

    if (cam.mMode == CameraMode::FPS) {
        float forward = 0.0f, right = 0.0f, up = 0.0f;
        if (mInputManager.isMoveForward())  forward += moveSpeed;
        if (mInputManager.isMoveBackward()) forward -= moveSpeed;
        if (mInputManager.isMoveRight())    right += moveSpeed;
        if (mInputManager.isMoveLeft())     right -= moveSpeed;
        if (mInputManager.isMoveUp())       up += moveSpeed;
        if (mInputManager.isMoveDown())     up -= moveSpeed;
        if (forward != 0.0f || right != 0.0f || up != 0.0f) {
            cam.moveFPS(forward, right, up);
        }
    } else {
        if (mInputManager.isMoveUp())       cam.zoomOrbit(-moveSpeed * 2);
        if (mInputManager.isMoveDown())     cam.zoomOrbit(moveSpeed * 2);
    }

    cam.update();
}

void Game::Update(float deltaTime) {
    for (auto& body : mPlanets) {
        body.data.angle += body.data.orbitSpeed * deltaTime;
        body.data.rotationAngle += body.data.rotationSpeed * deltaTime;
        body.data.moon.angle += body.data.moon.orbitSpeed * deltaTime;
        body.data.moon.rotationAngle += body.data.moon.rotationSpeed * deltaTime;
    }
}

void Game::Render(float deltaTime) {
    float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
    mRenderer.beginFrame(clearColor);

    auto& cam = mRenderer.getCamera();
    auto view = cam.getView();
    auto proj = cam.getProjection();

    {
        DirectX::XMMATRIX sunWorld = DirectX::XMMatrixRotationY(0.0f) *
                                      DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f);
        draw(mSunObj, sunWorld, view, proj, mRenderer);
    }

    for (auto& body : mPlanets) {
        DirectX::XMMATRIX orbitWorld = DirectX::XMMatrixScaling(body.data.orbitRadius, 1.0f, body.data.orbitRadius) *
                                        DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f);
        draw(body.orbitObj, orbitWorld, view, proj, mRenderer);

        float px = body.data.orbitRadius * cosf(body.data.angle);
        float pz = body.data.orbitRadius * sinf(body.data.angle);

        DirectX::XMMATRIX planetWorld = DirectX::XMMatrixScaling(body.data.size, body.data.size, body.data.size) *
                                         DirectX::XMMatrixRotationY(body.data.rotationAngle) *
                                         DirectX::XMMatrixTranslation(px, 0.0f, pz);
        draw(body.planetObj, planetWorld, view, proj, mRenderer);

        float mx = px + body.data.moon.orbitRadius * cosf(body.data.moon.angle);
        float mz = pz + body.data.moon.orbitRadius * sinf(body.data.moon.angle);

        DirectX::XMMATRIX moonWorld = DirectX::XMMatrixScaling(body.data.moon.size, body.data.moon.size, body.data.moon.size) *
                                       DirectX::XMMatrixRotationY(body.data.moon.rotationAngle) *
                                       DirectX::XMMatrixTranslation(mx, 0.0f, mz);
        draw(body.moonObj, moonWorld, view, proj, mRenderer);
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    {
        auto& cam = mRenderer.getCamera();
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Reload Planets")) {
                    reloadPlanets();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Projection")) {
                bool persp = (cam.getProjMode() == ProjMode::Persp);
                if (ImGui::MenuItem("Perspective", nullptr, persp))
                    cam.setProjMode(ProjMode::Persp);
                if (persp) {
                    float fov = cam.getFov();
                    if (ImGui::SliderFloat("FOV", &fov, 45.0f, 100.0f, "%.0f deg"))
                        cam.setFov(fov);
                }
                if (ImGui::MenuItem("Orthographic", nullptr, !persp))
                    cam.setProjMode(ProjMode::Ortho);
                ImGui::EndMenu();
            }
            if (!mReloadStatus.empty()) {
                ImGui::SameLine();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextUnformatted(mReloadStatus.c_str());
            }
            ImGui::EndMainMenuBar();
        }
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    mRenderer.endFrame();
}
