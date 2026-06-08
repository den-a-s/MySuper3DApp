#include "KatamariAtHomeState.h"
#include "Game.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <windows.h>
#include <algorithm>
#include <comdef.h>
#include <vector>
#include <fstream>
#include <iomanip>
#include <json.hpp>

#pragma comment(lib, "windowscodecs.lib")

static Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> createCheckerTexture(
    ID3D11Device* device, int size, int tileSize)
{
    std::vector<uint8_t> pixels(size * size * 4);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            int tx = x / tileSize;
            int ty = y / tileSize;
            bool light = (tx + ty) % 2 == 0;
            int idx = (y * size + x) * 4;
            uint8_t v = light ? 210 : 140;
            pixels[idx + 0] = v;
            pixels[idx + 1] = v;
            pixels[idx + 2] = v;
            pixels[idx + 3] = 255;
        }
    }

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = size;
    td.Height = size;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sd = {};
    sd.pSysMem = pixels.data();
    sd.SysMemPitch = size * 4;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
    if (FAILED(device->CreateTexture2D(&td, &sd, tex.GetAddressOf())))
        return nullptr;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    if (FAILED(device->CreateShaderResourceView(tex.Get(), nullptr, srv.GetAddressOf())))
        return nullptr;

    return srv;
}

static LoadedMesh createGroundMesh(float size, float uTiles) {
    LoadedMesh mesh;
    float h = size * 0.5f;
    VertexPNT verts[] = {
        {{-h, 0.0f, -h}, {0,1,0}, {0, 0}},
        {{ h, 0.0f, -h}, {0,1,0}, {uTiles, 0}},
        {{ h, 0.0f,  h}, {0,1,0}, {uTiles, uTiles}},
        {{-h, 0.0f,  h}, {0,1,0}, {0, uTiles}},
    };
    mesh.vertices.assign(verts, verts + 4);
    mesh.indices = {0, 1, 2, 0, 2, 3};
    return mesh;
}

KatamariAtHomeState::KatamariAtHomeState(Game& game) : mGame(game) {}

void KatamariAtHomeState::init() {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    auto& renderer = mGame.getRenderer();
    auto* device = renderer.mDevice.Get();

    auto groundChecker = createCheckerTexture(device, 256, 32);
    auto groundMesh = createGroundMesh(40.0f, 16.0f);

    LoadedMaterial groundMat;
    groundMat.baseColor = {0.8f, 0.8f, 0.8f, 1.0f};
    groundMat.ambientColor = {0.5f, 0.5f, 0.5f, 1.0f};

    mGround = TexturedRenderObj::create(renderer, L"../Shaders/ObjectShader.hlsl",
                                         groundMesh, groundMat, "");
    mGround.mAlbedoSRV = groundChecker;

    loadScene();
}

void KatamariAtHomeState::loadScene() {
    using json = nlohmann::json;

    std::ifstream f(mScenePath);
    if (!f.is_open()) {
        log("scene.json not found, using defaults");
        return;
    }

    json j;
    try { f >> j; } catch (...) {
        log("Failed to parse scene.json");
        return;
    }

    auto& renderer = mGame.getRenderer();
    mObjects.clear();
    log("Loading scene...");

    auto& objs = j["objects"];
    int idx = 1;
    for (auto& item : objs) {
        std::string objPath = item["objPath"];

        auto subMeshes = parseObj(objPath);

        KatamariObject obj;
        for (auto& sub : subMeshes) {
            obj.renderObjs.push_back(
                TexturedRenderObj::create(
                    renderer, L"../Shaders/ObjectShader.hlsl",
                    sub.mesh, sub.material, "../Objects/Textures"));
        }

        obj.position = {0, 0, 0};
        obj.rotation = {0, 0, 0};
        obj.scale = 1.0f;
        obj.scaleMultiplier = 1.0f;
        obj.name = item["name"];
        obj.objPath = objPath;

        if (item.contains("position")) {
            auto& p = item["position"];
            obj.position = {p[0], p[1], p[2]};
        }
        if (item.contains("rotation")) {
            auto& r = item["rotation"];
            obj.rotation = {r[0], r[1], r[2]};
        }
        if (item.contains("scale"))
            obj.scale = item["scale"];
        if (item.contains("multiplier"))
            obj.scaleMultiplier = item["multiplier"];

        mObjects.push_back(std::move(obj));
        log("[" + std::to_string(idx++) + "] Loaded " + objPath);
    }
    log("Scene loaded (" + std::to_string(mObjects.size()) + " objects)");
}

void KatamariAtHomeState::saveScene() {
    using json = nlohmann::json;

    json j = json::object();
    json arr = json::array();
    for (auto& o : mObjects) {
        json item;
        item["name"] = o.name;
        item["objPath"] = o.objPath;
        item["position"] = {o.position.x, o.position.y, o.position.z};
        item["rotation"] = {o.rotation.x, o.rotation.y, o.rotation.z};
        item["scale"] = o.scale;
        item["multiplier"] = o.scaleMultiplier;
        arr.push_back(item);
    }
    j["objects"] = arr;

    std::ofstream f(mScenePath);
    if (!f.is_open()) {
        mSceneStatus = "Failed to open scene.json for writing";
        return;
    }
    f << std::setw(4) << j << std::endl;
    log("Scene saved (" + std::to_string(mObjects.size()) + " objects)");
}

void KatamariAtHomeState::onEnter() {
    auto& renderer = mGame.getRenderer();
    float aspect = (float)renderer.mDisplay->getScreenWidth() /
                   (float)renderer.mDisplay->getScreenHeight();
    mInitialAspect = aspect;
    auto& cam = renderer.getCamera();
    cam.initFPS(DirectX::XMFLOAT3(0.0f, 8.0f, -18.0f), 0.0f, -0.4f, aspect);
    cam.setProjMode(ProjMode::Persp);
    SetWindowText(renderer.mDisplay->getHandlerWindow(), L"Katamari At Home");
}

void KatamariAtHomeState::resetCamera() {
    auto& renderer = mGame.getRenderer();
    auto& cam = renderer.getCamera();
    cam.initFPS(DirectX::XMFLOAT3(0.0f, 8.0f, -18.0f), 0.0f, -0.4f, mInitialAspect);
    cam.setProjMode(ProjMode::Persp);
}

void KatamariAtHomeState::handleInput(float deltaTime) {
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
                if (cam.mMode == CameraMode::FPS)
                    cam.rotateFPS(dx, dy);
                else
                    cam.rotateOrbit(dx, dy);
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
            if (cam.mMode == CameraMode::FPS)
                cam.setMode(CameraMode::Orbit);
            else
                cam.setMode(CameraMode::FPS);
            cam.update();
        }
    } else {
        mCamSwitchHeld = false;
    }

    float moveSpeed = mMoveSpeed * deltaTime;
    auto& cam = mGame.getRenderer().getCamera();

    if (cam.mMode == CameraMode::FPS) {
        float forward = 0.0f, right = 0.0f, up = 0.0f;
        if (mInput.isMoveForward())  forward += moveSpeed;
        if (mInput.isMoveBackward()) forward -= moveSpeed;
        if (mInput.isMoveRight())    right += moveSpeed;
        if (mInput.isMoveLeft())     right -= moveSpeed;
        if (mInput.isMoveUp())       up += moveSpeed;
        if (mInput.isMoveDown())     up -= moveSpeed;
        if (forward != 0.0f || right != 0.0f || up != 0.0f)
            cam.moveFPS(forward, right, up);
    } else {
        if (mInput.isMoveUp())       cam.zoomOrbit(-moveSpeed * 2);
        if (mInput.isMoveDown())     cam.zoomOrbit(moveSpeed * 2);
    }

    cam.update();
}

void KatamariAtHomeState::update(float) {}

void KatamariAtHomeState::log(const std::string& msg) {
    mLog.push_back(msg);
    if (mLog.size() > 50) mLog.pop_front();
}

DirectX::XMFLOAT4 KatamariAtHomeState::getCameraPosition() const {
    auto& cam = mGame.getRenderer().getCamera();
    DirectX::XMVECTOR det;
    auto invView = DirectX::XMMatrixInverse(&det, cam.getView());
    DirectX::XMFLOAT4 pos;
    DirectX::XMStoreFloat4(&pos, invView.r[3]);
    return pos;
}

void KatamariAtHomeState::render(float deltaTime) {
    auto& renderer = mGame.getRenderer();
    float clearColor[] = {0.15f, 0.15f, 0.25f, 1.0f};
    renderer.beginFrame(clearColor);

    auto& cam = renderer.getCamera();
    auto view = cam.getView();
    auto proj = cam.getProjection();
    auto camPos = getCameraPosition();

    {
        DirectX::XMMATRIX groundWorld = DirectX::XMMatrixIdentity();
        drawTextured(mGround, groundWorld, view, proj, camPos, renderer);
    }

    for (size_t i = 0; i < mObjects.size(); ++i) {
        auto& obj = mObjects[i];
        float s = obj.scale * obj.scaleMultiplier;
        DirectX::XMMATRIX world =
            DirectX::XMMatrixScaling(s, s, s) *
            DirectX::XMMatrixRotationX(obj.rotation.x) *
            DirectX::XMMatrixRotationY(obj.rotation.y) *
            DirectX::XMMatrixRotationZ(obj.rotation.z) *
            DirectX::XMMatrixTranslation(obj.position.x, obj.position.y, obj.position.z);
        for (auto& renderObj : obj.renderObjs) {
            drawTextured(renderObj, world, view, proj, camPos, renderer);
        }
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    {
        auto& camRef = renderer.getCamera();
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit"))
                    mGame.switchState(GameStateType::Menu);
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
            if (ImGui::BeginMenu("Settings")) {
                ImGui::SliderFloat("Move Speed", &mMoveSpeed, 0.5f, 30.0f, "%.1f");
                if (ImGui::MenuItem("Reset Camera"))
                    resetCamera();
                ImGui::Separator();
                if (ImGui::BeginMenu("Object Transforms")) {
                    for (size_t i = 0; i < mObjects.size(); ++i) {
                        auto& obj = mObjects[i];

                        auto slashPos = obj.objPath.find_last_of('/');
                        std::string fname = (slashPos != std::string::npos) ? obj.objPath.substr(slashPos + 1) : obj.objPath;
                        std::string label = std::to_string(i + 1) + " (" + fname + ")";

                        if (ImGui::BeginMenu(label.c_str())) {
                            float pos[2] = {obj.position.x, obj.position.z};
                            if (ImGui::DragFloat2("Pos XZ", pos, 0.1f)) {
                                obj.position.x = pos[0];
                                obj.position.z = pos[1];
                            }

                            float rotY = obj.rotation.y;
                            if (ImGui::SliderAngle("Rot Y", &rotY, -180.0f, 180.0f)) {
                                obj.rotation.y = rotY;
                                log("[" + std::to_string(i+1) + "] Rot → " +
                                    std::to_string(rotY) + " rad");
                            }

                            ImGui::Text("Base Scale: %.3f", obj.scale);
                            if (ImGui::DragFloat("Scale Mult", &obj.scaleMultiplier,
                                0.01f, 0.01f, 10.0f, "%.2f"))
                                log("[" + std::to_string(i+1) + "] Scale → " +
                                    std::to_string(obj.scaleMultiplier));

                            ImGui::EndMenu();
                        }
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Save to file"))
                        saveScene();
                    if (ImGui::MenuItem("Load from file"))
                        loadScene();
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (!mSceneStatus.empty()) {
                ImGui::SameLine();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextUnformatted(mSceneStatus.c_str());
            }
            if (ImGui::BeginMenu("Help")) {
                ImGui::TextUnformatted("WASD       - Move/pan camera");
                ImGui::TextUnformatted("Space/Shift - Move up/down");
                ImGui::TextUnformatted("C          - Switch camera mode");
                ImGui::Separator();
                ImGui::TextUnformatted("LMB + drag - Rotate camera");
                ImGui::Separator();
                ImGui::TextUnformatted("ESC - Back to Menu");
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    {
        ImGui::Begin("FPS", nullptr,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground |
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
        ImGui::SetWindowPos(ImVec2(10, 24), ImGuiCond_Always);
        ImGui::SetWindowFontScale(1.0f);
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "FPS: %.0f", 1.0f / deltaTime);
        ImGui::End();
    }

    {
        auto display = ImGui::GetIO().DisplaySize;
        ImGui::Begin("##log", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);
        ImGui::SetWindowPos(ImVec2(0, display.y - 60), ImGuiCond_Always);
        ImGui::SetWindowSize(ImVec2(display.x, 60), ImGuiCond_Always);
        for (auto& line : mLog)
            ImGui::TextUnformatted(line.c_str());
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    renderer.endFrame();
}
