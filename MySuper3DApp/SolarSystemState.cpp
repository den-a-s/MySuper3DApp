#include "SolarSystemState.h"
#include "Game.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <windows.h>
#include <cstdio>
#include <algorithm>
#include <fstream>
#include <json.hpp>

SolarSystemState::SolarSystemState(Game& game) : mGame(game) {}

void SolarSystemState::init() {
    auto sunColor = DirectX::XMFLOAT4(1.0f, 0.85f, 0.3f, 1.0f);
    auto sunMesh = createSphereMeshData(sunColor, 0.6f, 16, 12);
    auto& renderer = mGame.getRenderer();
    mSunObj = SquareRenderObj::create(renderer, L"../Shaders/MyVeryFirstShader.hlsl", sunMesh);

    reloadPlanets();
}

void SolarSystemState::onEnter() {
    auto& renderer = mGame.getRenderer();
    float aspect = (float)renderer.mDisplay->getScreenWidth() /
                   (float)renderer.mDisplay->getScreenHeight();
    auto& cam = renderer.getCamera();
    cam.initFPS(DirectX::XMFLOAT3(0.0f, 20.0f, 0.0f), 0.0f, -1.55f, aspect);
    cam.setProjMode(ProjMode::Persp);
    SetWindowText(renderer.mDisplay->getHandlerWindow(), L"MyFirst3DApp");
}

void SolarSystemState::handleInput(float deltaTime) {
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        switch (msg.message) {
        case WM_MOUSEMOVE:
            if (mMousePressed) {
                int x = static_cast<int>(LOWORD(msg.lParam));
                int y = static_cast<int>(HIWORD(msg.lParam));
                float dx = static_cast<float>(x - mMouseX) * 0.005f;
                float dy = static_cast<float>(mMouseY - y) * 0.005f;
                auto& cam = mGame.getRenderer().getCamera();
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
            mGame.requestExit();
            return;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (mInput.isExit()) {
        mGame.switchState(GameStateType::Menu);
        return;
    }

    if (mInput.isSwitchCamera()) {
        if (!mCamSwitchHeld) {
            mCamSwitchHeld = true;
            auto& cam = mGame.getRenderer().getCamera();
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
    auto& cam = mGame.getRenderer().getCamera();

    if (cam.mMode == CameraMode::FPS) {
        float forward = 0.0f, right = 0.0f, up = 0.0f;
        if (mInput.isMoveForward())  forward += moveSpeed;
        if (mInput.isMoveBackward()) forward -= moveSpeed;
        if (mInput.isMoveRight())    right += moveSpeed;
        if (mInput.isMoveLeft())     right -= moveSpeed;
        if (mInput.isMoveUp())       up += moveSpeed;
        if (mInput.isMoveDown())     up -= moveSpeed;
        if (forward != 0.0f || right != 0.0f || up != 0.0f) {
            cam.moveFPS(forward, right, up);
        }
    } else {
        if (mInput.isMoveUp())       cam.zoomOrbit(-moveSpeed * 2);
        if (mInput.isMoveDown())     cam.zoomOrbit(moveSpeed * 2);
    }

    cam.update();
}

void SolarSystemState::update(float deltaTime) {
    for (auto& body : mPlanets) {
        body.data.angle += body.data.orbitSpeed * deltaTime;
        body.data.rotationAngle += body.data.rotationSpeed * deltaTime;
        body.data.moon.angle += body.data.moon.orbitSpeed * deltaTime;
        body.data.moon.rotationAngle += body.data.moon.rotationSpeed * deltaTime;
    }
}

void SolarSystemState::reloadPlanets() {
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
    auto& renderer = mGame.getRenderer();

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
            body.planetObj = SquareRenderObj::create(renderer, L"../Shaders/MyVeryFirstShader.hlsl", mesh);
        } else {
            auto mesh = createSphereMeshData(body.data.color, 1.0f, 16, 12);
            body.planetObj = SquareRenderObj::create(renderer, L"../Shaders/MyVeryFirstShader.hlsl", mesh);
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
            body.moonObj = SquareRenderObj::create(renderer, L"../Shaders/MyVeryFirstShader.hlsl", mesh);
        } else {
            auto mesh = createSphereMeshData(body.data.moon.color, 1.0f, 12, 8);
            body.moonObj = SquareRenderObj::create(renderer, L"../Shaders/MyVeryFirstShader.hlsl", mesh);
        }

        auto ringMesh = createRingMeshData(white, 0.98f, 1.02f, 48);
        body.orbitObj = SquareRenderObj::create(renderer, L"../Shaders/MyVeryFirstShader.hlsl", ringMesh);

        mPlanets.push_back(std::move(body));
    }

    mReloadStatus = "Reloaded " + std::to_string(mPlanets.size()) + " planets";
}

void SolarSystemState::render(float deltaTime) {
    auto& renderer = mGame.getRenderer();
    float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
    renderer.beginFrame(clearColor);

    auto& cam = renderer.getCamera();
    auto view = cam.getView();
    auto proj = cam.getProjection();

    {
        DirectX::XMMATRIX sunWorld = DirectX::XMMatrixRotationY(0.0f) *
                                      DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f);
        draw(mSunObj, sunWorld, view, proj, renderer);
    }

    for (auto& body : mPlanets) {
        DirectX::XMMATRIX orbitWorld = DirectX::XMMatrixScaling(body.data.orbitRadius, 1.0f, body.data.orbitRadius) *
                                        DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f);
        draw(body.orbitObj, orbitWorld, view, proj, renderer);

        float px = body.data.orbitRadius * cosf(body.data.angle);
        float pz = body.data.orbitRadius * sinf(body.data.angle);

        DirectX::XMMATRIX planetWorld = DirectX::XMMatrixScaling(body.data.size, body.data.size, body.data.size) *
                                         DirectX::XMMatrixRotationY(body.data.rotationAngle) *
                                         DirectX::XMMatrixTranslation(px, 0.0f, pz);
        draw(body.planetObj, planetWorld, view, proj, renderer);

        float mx = px + body.data.moon.orbitRadius * cosf(body.data.moon.angle);
        float mz = pz + body.data.moon.orbitRadius * sinf(body.data.moon.angle);

        DirectX::XMMATRIX moonWorld = DirectX::XMMatrixScaling(body.data.moon.size, body.data.moon.size, body.data.moon.size) *
                                       DirectX::XMMatrixRotationY(body.data.moon.rotationAngle) *
                                       DirectX::XMMatrixTranslation(mx, 0.0f, mz);
        draw(body.moonObj, moonWorld, view, proj, renderer);
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    {
        auto& camRef = renderer.getCamera();
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Reload Planets")) {
                    reloadPlanets();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit")) {
                    mGame.switchState(GameStateType::Menu);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Projection")) {
                bool persp = (camRef.getProjMode() == ProjMode::Persp);
                if (ImGui::MenuItem("Perspective", nullptr, persp))
                    camRef.setProjMode(ProjMode::Persp);
                if (persp) {
                    float fov = camRef.getFov();
                    if (ImGui::SliderFloat("FOV", &fov, 45.0f, 100.0f, "%.0f deg"))
                        camRef.setFov(fov);
                }
                if (ImGui::MenuItem("Orthographic", nullptr, !persp))
                    camRef.setProjMode(ProjMode::Ortho);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                ImGui::TextUnformatted("WASD       - Move camera");
                ImGui::TextUnformatted("Space/Shift - Move up/down");
                ImGui::TextUnformatted("C          - Switch camera mode");
                ImGui::Separator();
                ImGui::TextUnformatted("LMB + Mouse drag - Rotate camera");
                ImGui::Separator();
                ImGui::TextUnformatted("ESC - Back to Menu");
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

    renderer.endFrame();
}
