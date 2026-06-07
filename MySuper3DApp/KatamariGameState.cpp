#include "KatamariGameState.h"
#include "Game.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <windows.h>
#include <algorithm>
#include <cmath>
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

KatamariGameState::KatamariGameState(Game& game) : mGame(game), mRng(std::random_device{}()) {
    mBallOrientation = DirectX::XMQuaternionIdentity();
    mPrevCamForward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
}

void KatamariGameState::init() {
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

    for (size_t i = 0; i < mObjects.size(); ++i) {
        if (mObjects[i].isBall) {
            mBallIndex = (int)i;
            break;
        }
    }

    if (mBallIndex >= 0) {
        mBallBaseRadius = mObjects[mBallIndex].boundingRadius;
        mBallRadius = mBallBaseRadius;
        mCameraDistance = max(mCameraDistance, mBallRadius * 3.0f);
        scatterObjects();
        log("Game started — " + std::to_string(mObjects.size() - 1) + " objects scattered");
    } else {
        log("ERROR: No ball object found in scene.json");
    }
}

void KatamariGameState::loadScene() {
    using json = nlohmann::json;

    std::ifstream f(mScenePath);
    if (!f.is_open()) {
        log("scene.json not found");
        return;
    }

    json j;
    try { f >> j; } catch (...) {
        log("Failed to parse scene.json");
        return;
    }

    auto& renderer = mGame.getRenderer();
    mObjects.clear();

    auto& objs = j["objects"];
    for (auto& item : objs) {
        std::string objPath = item["objPath"];

        std::vector<LoadedMaterial> mats;
        auto mesh = parseObj(objPath, mats);

        KatamariGameObject obj;
        if (!mesh.vertices.empty()) {
            float cx = 0, cy = 0, cz = 0;
            for (auto& v : mesh.vertices) {
                cx += v.position.x;
                cy += v.position.y;
                cz += v.position.z;
            }
            float invN = 1.0f / (float)mesh.vertices.size();
            cx *= invN; cy *= invN; cz *= invN;
            for (auto& v : mesh.vertices) {
                v.position.x -= cx;
                v.position.y -= cy;
                v.position.z -= cz;
            }

            LoadedMaterial mat;
            if (!mats.empty())
                mat = mats.back();
            obj.renderObj = TexturedRenderObj::create(
                renderer, L"../Shaders/ObjectShader.hlsl",
                mesh, mat, "../Objects/Textures");

            float maxR = 0.0f;
            for (auto& v : mesh.vertices) {
                float r = sqrtf(v.position.x * v.position.x +
                                v.position.y * v.position.y +
                                v.position.z * v.position.z);
                if (r > maxR) maxR = r;
            }
            obj.boundingRadius = maxR;
        }

        obj.position = {0, 0, 0};
        obj.rotation = {0, 0, 0};
        obj.scale = 1.0f;
        obj.scaleMultiplier = 1.0f;
        obj.name = item["name"];
        obj.objPath = objPath;
        obj.isBall = item.value("isBall", false);

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
    }
    log("Scene loaded (" + std::to_string(mObjects.size()) + " objects)");
}

void KatamariGameState::scatterObjects() {
    if (mBallIndex < 0) return;
    auto& ballPos = mObjects[mBallIndex].position;

    std::uniform_real_distribution<float> dist(-100.0f, 100.0f);

    for (int i = 0; i < (int)mObjects.size(); ++i) {
        if (i == mBallIndex) continue;
        auto& obj = mObjects[i];
        float r = obj.boundingRadius * obj.scale * obj.scaleMultiplier;
        obj.position.x = dist(mRng);
        obj.position.z = dist(mRng);
        obj.position.y = r;
    }
}

DirectX::XMVECTOR KatamariGameState::computeObjectWorldPos(int index, DirectX::XMVECTOR ballPos) const {
    if (index == mBallIndex)
        return ballPos;

    auto& obj = mObjects[index];
    DirectX::XMVECTOR localOffset = DirectX::XMLoadFloat3(&obj.attachOffset);
    DirectX::XMVECTOR worldOffset = DirectX::XMVector3Rotate(localOffset, mBallOrientation);

    if (obj.parentIndex == -2) {
        return DirectX::XMVectorAdd(ballPos, worldOffset);
    } else if (obj.parentIndex >= 0) {
        DirectX::XMVECTOR parentPos = computeObjectWorldPos(obj.parentIndex, ballPos);
        return DirectX::XMVectorAdd(parentPos, worldOffset);
    }
    return ballPos;
}

void KatamariGameState::onEnter() {
    auto& renderer = mGame.getRenderer();
    float aspect = (float)renderer.mDisplay->getScreenWidth() /
                   (float)renderer.mDisplay->getScreenHeight();
    mInitialAspect = aspect;
    auto& cam = renderer.getCamera();

    if (mBallIndex >= 0) {
        cam.initOrbit(aspect, 0.5f, mCameraDistance);
        cam.setMode(CameraMode::Orbit);
        cam.setTarget(mObjects[mBallIndex].position);
    } else {
        cam.initFPS(DirectX::XMFLOAT3(0.0f, 8.0f, -18.0f), 0.0f, -0.4f, aspect);
    }
    SetWindowText(renderer.mDisplay->getHandlerWindow(), L"Katamari Game");
}

void KatamariGameState::handleInput(float deltaTime) {
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
        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
            auto& cam = mGame.getRenderer().getCamera();
            cam.zoomOrbit(-delta / 120.0f * 2.0f);
            break;
        }
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
}

void KatamariGameState::update(float dt) {
    updateBall(dt);
    checkPickups();

    if (mBallIndex >= 0) {
        auto& cam = mGame.getRenderer().getCamera();
        cam.setTarget(mObjects[mBallIndex].position);
        cam.update();
    }
}

void KatamariGameState::updateBall(float dt) {
    if (mBallIndex < 0) return;
    auto& ball = mObjects[mBallIndex];

    auto& cam = mGame.getRenderer().getCamera();
    DirectX::XMVECTOR ballPos = DirectX::XMLoadFloat3(&ball.position);

    DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(nullptr, cam.getView());
    DirectX::XMVECTOR eye = invView.r[3];

    DirectX::XMVECTOR toTarget = DirectX::XMVectorSubtract(ballPos, eye);
    DirectX::XMVECTOR forward = DirectX::XMVectorSetY(toTarget, 0.0f);
    forward = DirectX::XMVector3Normalize(forward);
    DirectX::XMVECTOR right = DirectX::XMVector3Cross(
        DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), forward);

    // Camera Y-rotation conjugate (reproject accumulated roll to new camera axes)
    float prevYaw = atan2f(DirectX::XMVectorGetZ(mPrevCamForward),
                           DirectX::XMVectorGetX(mPrevCamForward));
    float curYaw = atan2f(DirectX::XMVectorGetZ(forward),
                          DirectX::XMVectorGetX(forward));
    float yawDelta = curYaw - prevYaw;
    constexpr float kPI = 3.14159265f;
    constexpr float k2PI = 6.28318531f;
    if (yawDelta > kPI) yawDelta -= k2PI;
    if (yawDelta < -kPI) yawDelta += k2PI;
    if (fabsf(yawDelta) > 0.0001f) {
        DirectX::XMVECTOR yRot = DirectX::XMQuaternionRotationAxis(
            DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), yawDelta);
        DirectX::XMVECTOR yRotInv = DirectX::XMQuaternionInverse(yRot);
        mBallOrientation = DirectX::XMQuaternionMultiply(yRot,
            DirectX::XMQuaternionMultiply(mBallOrientation, yRotInv));
        mBallOrientation = DirectX::XMQuaternionNormalize(mBallOrientation);
    }
    mPrevCamForward = forward;

    DirectX::XMVECTOR moveDir = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    if (mInput.isMoveForward())  moveDir = DirectX::XMVectorAdd(moveDir, forward);
    if (mInput.isMoveBackward()) moveDir = DirectX::XMVectorSubtract(moveDir, forward);
    if (mInput.isMoveRight())    moveDir = DirectX::XMVectorAdd(moveDir, right);
    if (mInput.isMoveLeft())     moveDir = DirectX::XMVectorSubtract(moveDir, right);

    float moveLen = DirectX::XMVectorGetX(DirectX::XMVector3Length(moveDir));
    if (moveLen > 0.001f) {
        moveDir = DirectX::XMVectorScale(moveDir, 1.0f / moveLen);

        float dx = DirectX::XMVectorGetX(moveDir) * mMoveSpeed * dt;
        float dz = DirectX::XMVectorGetZ(moveDir) * mMoveSpeed * dt;
        ball.position.x += dx;
        ball.position.z += dz;

        // Rolling visual in world frame
        DirectX::XMVECTOR axis = DirectX::XMVector3Cross(
            DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), moveDir);
        float axisLen = DirectX::XMVectorGetX(DirectX::XMVector3Length(axis));
        if (axisLen > 0.0001f) {
            axis = DirectX::XMVectorScale(axis, 1.0f / axisLen);
            float angle = mMoveSpeed * dt / mBallRadius;
            DirectX::XMVECTOR deltaQ = DirectX::XMQuaternionRotationAxis(axis, angle);
            mBallOrientation = DirectX::XMQuaternionMultiply(deltaQ, mBallOrientation);
            mBallOrientation = DirectX::XMQuaternionNormalize(mBallOrientation);
        }
    }

    ball.position.y = mBallRadius;
}

void KatamariGameState::checkPickups() {
    if (mBallIndex < 0) return;
    DirectX::XMVECTOR ballPos = DirectX::XMLoadFloat3(&mObjects[mBallIndex].position);

    for (int i = 0; i < (int)mObjects.size(); ++i) {
        if (i == mBallIndex || mObjects[i].isAttached) continue;
        auto& obj = mObjects[i];

        DirectX::XMVECTOR objPos = DirectX::XMLoadFloat3(&obj.position);
        DirectX::XMVECTOR diff = DirectX::XMVectorSubtract(objPos, ballPos);
        float dist = DirectX::XMVectorGetX(DirectX::XMVector3Length(diff));
        float objR = obj.boundingRadius * obj.scale * obj.scaleMultiplier;
        float sumRadii = mBallRadius + objR;

        if (dist < sumRadii && objR < mBallRadius * 1.15f) {
            DirectX::XMVECTOR dir = DirectX::XMVector3Normalize(diff);
            if (DirectX::XMVectorGetX(DirectX::XMVector3Length(diff)) < 0.0001f) {
                dir = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
            }
            DirectX::XMVECTOR worldOffset = DirectX::XMVectorScale(dir, mBallRadius);
            DirectX::XMVECTOR invOrient = DirectX::XMQuaternionInverse(mBallOrientation);
            DirectX::XMVECTOR localOffset = DirectX::XMVector3Rotate(worldOffset, invOrient);
            DirectX::XMStoreFloat3(&obj.attachOffset, localOffset);
            obj.isAttached = true;
            obj.parentIndex = -2;
            mBallRadius = sqrtf(mBallRadius * mBallRadius + objR * objR);
            log("Picked up " + obj.name);
        }
    }

    // Chain pickups
    for (int i = 0; i < (int)mObjects.size(); ++i) {
        if (i == mBallIndex || mObjects[i].isAttached) continue;
        auto& freeObj = mObjects[i];
        DirectX::XMVECTOR freePos = DirectX::XMLoadFloat3(&freeObj.position);

        for (int j = 0; j < (int)mObjects.size(); ++j) {
            if (j == mBallIndex || !mObjects[j].isAttached) continue;

            DirectX::XMVECTOR attachedPos = computeObjectWorldPos(j, ballPos);
            DirectX::XMVECTOR diff = DirectX::XMVectorSubtract(freePos, attachedPos);
            float dist = DirectX::XMVectorGetX(DirectX::XMVector3Length(diff));
            float freeR = freeObj.boundingRadius * freeObj.scale * freeObj.scaleMultiplier;
            float attachedR = mObjects[j].boundingRadius * mObjects[j].scale * mObjects[j].scaleMultiplier;

            if (dist < freeR + attachedR && freeR < mBallRadius * 1.15f) {
                DirectX::XMVECTOR dir = DirectX::XMVector3Normalize(diff);
                if (DirectX::XMVectorGetX(DirectX::XMVector3Length(diff)) < 0.0001f) {
                    dir = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
                }
                DirectX::XMVECTOR worldOffset = DirectX::XMVectorScale(dir, attachedR);
                DirectX::XMVECTOR invOrient = DirectX::XMQuaternionInverse(mBallOrientation);
                DirectX::XMVECTOR localOffset = DirectX::XMVector3Rotate(worldOffset, invOrient);
                DirectX::XMStoreFloat3(&freeObj.attachOffset, localOffset);
                freeObj.isAttached = true;
                freeObj.parentIndex = j;
                mBallRadius = sqrtf(mBallRadius * mBallRadius + freeR * freeR);
                log("Picked up " + freeObj.name + " (chain)");
                break;
            }
        }
    }
}

void KatamariGameState::log(const std::string& msg) {
    mLog.push_back(msg);
    if (mLog.size() > 50) mLog.pop_front();
}

void KatamariGameState::render(float deltaTime) {
    auto& renderer = mGame.getRenderer();
    float clearColor[] = {0.15f, 0.15f, 0.25f, 1.0f};
    renderer.beginFrame(clearColor);

    auto& cam = renderer.getCamera();
    auto view = cam.getView();
    auto proj = cam.getProjection();
    auto camPos = [&]() {
        DirectX::XMVECTOR det;
        auto invView = DirectX::XMMatrixInverse(&det, view);
        DirectX::XMFLOAT4 p;
        DirectX::XMStoreFloat4(&p, invView.r[3]);
        return p;
    }();

    // Ground
    {
        DirectX::XMMATRIX groundWorld = DirectX::XMMatrixIdentity();
        drawTextured(mGround, groundWorld, view, proj, camPos, renderer);
    }

    // Ball
    if (mBallIndex >= 0) {
        auto& ball = mObjects[mBallIndex];
        float ballScale = mBallRadius / mBallBaseRadius;
        DirectX::XMMATRIX world =
            DirectX::XMMatrixScaling(ballScale, ballScale, ballScale) *
            DirectX::XMMatrixRotationQuaternion(mBallOrientation) *
            DirectX::XMMatrixTranslation(ball.position.x, ball.position.y, ball.position.z);
        drawTextured(ball.renderObj, world, view, proj, camPos, renderer);
    }

    // Attached + free objects
    DirectX::XMVECTOR ballPos = (mBallIndex >= 0)
        ? DirectX::XMLoadFloat3(&mObjects[mBallIndex].position)
        : DirectX::XMVectorSet(0, 0, 0, 0);

    for (size_t i = 0; i < mObjects.size(); ++i) {
        if ((int)i == mBallIndex) continue;
        auto& obj = mObjects[i];
        float s = obj.scale * obj.scaleMultiplier;

        DirectX::XMMATRIX world;
        if (obj.isAttached) {
            DirectX::XMVECTOR wpos = computeObjectWorldPos((int)i, ballPos);
            DirectX::XMFLOAT3 wp;
            DirectX::XMStoreFloat3(&wp, wpos);
            world =
                DirectX::XMMatrixScaling(s, s, s) *
                DirectX::XMMatrixRotationQuaternion(mBallOrientation) *
                DirectX::XMMatrixTranslation(wp.x, wp.y, wp.z);
        } else {
            world =
                DirectX::XMMatrixScaling(s, s, s) *
                DirectX::XMMatrixRotationX(obj.rotation.x) *
                DirectX::XMMatrixRotationY(obj.rotation.y) *
                DirectX::XMMatrixRotationZ(obj.rotation.z) *
                DirectX::XMMatrixTranslation(obj.position.x, obj.position.y, obj.position.z);
        }
        drawTextured(obj.renderObj, world, view, proj, camPos, renderer);
    }

    // UI
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit to Menu"))
                    mGame.switchState(GameStateType::Menu);
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
            if (ImGui::BeginMenu("Settings")) {
                ImGui::SliderFloat("Move Speed", &mMoveSpeed, 1.0f, 30.0f, "%.1f");
                ImGui::SliderFloat("Camera Distance", &mCameraDistance, 5.0f, 40.0f, "%.1f");
                ImGui::SliderFloat("Camera Height", &mCameraDistance, 5.0f, 40.0f, "%.1f");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                ImGui::TextUnformatted("WASD       - Roll the ball");
                ImGui::TextUnformatted("LMB + drag - Rotate camera");
                ImGui::TextUnformatted("Mouse wheel - Zoom");
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
        if (mBallIndex >= 0) {
            float displayRadius = mBallRadius;
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Radius: %.2f", displayRadius);
            int attached = 0;
            for (auto& o : mObjects)
                if (o.isAttached) attached++;
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Attached: %d", attached);
        }
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
