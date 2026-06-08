#include "KatamariAtHomeState.h"
#include "Game.h"
#include "KatamariCollisionSystem.h"
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
    mesh.indices = {0, 2, 1, 0, 3, 2};
    return mesh;
}

static LoadedMesh createSphereMesh(float radius, int slices, int stacks) {
    LoadedMesh mesh;
    for (int i = 0; i <= stacks; ++i) {
        float phi = DirectX::XM_PI * i / stacks;
        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * DirectX::XM_PI * j / slices;
            VertexPNT v;
            v.position.x = radius * sinf(phi) * cosf(theta);
            v.position.y = radius * cosf(phi);
            v.position.z = radius * sinf(phi) * sinf(theta);
            v.normal.x = sinf(phi) * cosf(theta);
            v.normal.y = cosf(phi);
            v.normal.z = sinf(phi) * sinf(theta);
            v.texcoord.x = (float)j / slices;
            v.texcoord.y = (float)i / stacks;
            mesh.vertices.push_back(v);
        }
    }
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int first = i * (slices + 1) + j;
            int second = first + slices + 1;
            mesh.indices.push_back(first);
            mesh.indices.push_back(second);
            mesh.indices.push_back(first + 1);
            mesh.indices.push_back(second);
            mesh.indices.push_back(second + 1);
            mesh.indices.push_back(first + 1);
        }
    }
    return mesh;
}

KatamariAtHomeState::KatamariAtHomeState(Game& game) : mGame(game), mRng(std::random_device{}()) {
}

KatamariAtHomeState::~KatamariAtHomeState() {
    CoUninitialize();
}

void KatamariAtHomeState::init() {
    std::ignore = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    auto& renderer = mGame.getRenderer();
    auto* device = renderer.mDevice.Get();

    auto groundChecker = createCheckerTexture(device, 256, 64);
    auto groundMesh = createGroundMesh(1200.0f, 12.0f);

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
        mBall.setBaseRadius(mObjects[mBallIndex].boundingRadius);
        mBall.setPosition(mObjects[mBallIndex].position);
        mCameraDistance = max(mCameraDistance, mBall.getRadius() * 3.0f);
        scatterObjects();
        log("Game started — " + std::to_string(mObjects.size() - 1) + " objects scattered");

        auto sphereMesh = createSphereMesh(1.0f, 16, 16);
        LoadedMaterial sphereMat;
        sphereMat.baseColor = {0.2f, 0.6f, 1.0f, 0.3f};
        sphereMat.ambientColor = {0.1f, 0.1f, 0.2f, 1.0f};
        mOutlineSphere = TexturedRenderObj::create(renderer, L"../Shaders/ObjectShader.hlsl",
                                                     sphereMesh, sphereMat, "");
        CD3D11_RASTERIZER_DESC outlineRastDesc = {};
        outlineRastDesc.CullMode = D3D11_CULL_NONE;
        outlineRastDesc.FillMode = D3D11_FILL_WIREFRAME;
        outlineRastDesc.DepthClipEnable = TRUE;
        if (FAILED(renderer.mDevice->CreateRasterizerState(&outlineRastDesc, mOutlineRasterizerState.GetAddressOf()))) {
            log("ERROR: Failed to create outline rasterizer state");
        }
        mOutlineSphere.mRasterizerState = mOutlineRasterizerState;
    } else {
        log("ERROR: No ball object found in scene.json");
    }
}

void KatamariAtHomeState::loadScene() {
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

        auto subMeshes = parseObj(objPath);

        KatamariGameObject obj;
        if (!subMeshes.empty()) {
            float cx = 0, cy = 0, cz = 0;
            int totalVerts = 0;
            for (auto& sub : subMeshes) {
                for (auto& v : sub.mesh.vertices) {
                    cx += v.position.x;
                    cy += v.position.y;
                    cz += v.position.z;
                }
                totalVerts += (int)sub.mesh.vertices.size();
            }
            if (totalVerts > 0) {
                float invN = 1.0f / (float)totalVerts;
                cx *= invN; cy *= invN; cz *= invN;
            }

            float maxR = 0.0f;
            for (auto& sub : subMeshes) {
                for (auto& v : sub.mesh.vertices) {
                    v.position.x -= cx;
                    v.position.y -= cy;
                    v.position.z -= cz;
                }
                for (auto& v : sub.mesh.vertices) {
                    float r = sqrtf(v.position.x * v.position.x +
                                    v.position.y * v.position.y +
                                    v.position.z * v.position.z);
                    if (r > maxR) maxR = r;
                }
                obj.renderObjs.push_back(
                    TexturedRenderObj::create(
                        renderer, L"../Shaders/ObjectShader.hlsl",
                        sub.mesh, sub.material, "../Objects/Textures"));
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

        obj.gameSize = item.value("katSize", obj.boundingRadius * obj.scale * obj.scaleMultiplier);

        mObjects.push_back(std::move(obj));
    }
    log("Scene loaded (" + std::to_string(mObjects.size()) + " objects)");
}

void KatamariAtHomeState::scatterObjects() {
    if (mBallIndex < 0) return;
    auto& ballPos = mObjects[mBallIndex].position;

    std::uniform_real_distribution<float> dist(-550.0f, 550.0f);

    for (int i = 0; i < (int)mObjects.size(); ++i) {
        if (i == mBallIndex) continue;
        auto& obj = mObjects[i];
        float r = obj.boundingRadius * obj.scale * obj.scaleMultiplier;
        obj.position.x = dist(mRng);
        obj.position.z = dist(mRng);
        obj.position.y = r;
    }
}

void KatamariAtHomeState::onEnter() {
    auto& renderer = mGame.getRenderer();
    float aspect = (float)renderer.mDisplay->getScreenWidth() /
                   (float)renderer.mDisplay->getScreenHeight();
    mInitialAspect = aspect;
    auto& cam = renderer.getCamera();

    if (mBallIndex >= 0) {
        cam.initOrbit(aspect, 0.5f, mCameraDistance);
        cam.setMode(CameraMode::Orbit);
        cam.setTarget(mBall.getPosition());
    } else {
        cam.initFPS(DirectX::XMFLOAT3(0.0f, 8.0f, -18.0f), 0.0f, -0.4f, aspect);
    }
    mBall.resetMotion();
    SetWindowText(renderer.mDisplay->getHandlerWindow(), L"Katamari At Home");
}

void KatamariAtHomeState::handleInput(float deltaTime) {
    mInput.pumpMessages();

    if (mInput.isExitRequested()) {
        mGame.requestExit();
        return;
    }

    if (mInput.isExit()) {
        mGame.switchState(GameStateType::Menu);
        return;
    }

    if (mInput.isMouseDown() && !ImGui::GetIO().WantCaptureMouse) {
        auto& cam = mGame.getRenderer().getCamera();
        cam.rotateOrbit(mInput.getMouseDeltaX(), mInput.getMouseDeltaY());
    }
    auto& cam = mGame.getRenderer().getCamera();
    cam.zoomOrbit(mInput.getScrollDelta());
}

void KatamariAtHomeState::update(float dt) {
    mBall.update(dt, mInput, mGame.getRenderer().getCamera());

    katamariCheckPickups(mObjects, mBallIndex, mBall, [this](const std::string& msg) {
        log(msg);
    });

    if (mBallIndex >= 0) {
        mObjects[mBallIndex].position = mBall.getPosition();
        auto& cam = mGame.getRenderer().getCamera();
        cam.setTarget(mObjects[mBallIndex].position);
        cam.update();
    }
}

void KatamariAtHomeState::log(const std::string& msg) {
    mLog.push_back(msg);
    if (mLog.size() > 50) mLog.pop_front();
}

void KatamariAtHomeState::render(float deltaTime) {
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

    {
        DirectX::XMMATRIX groundWorld = DirectX::XMMatrixIdentity();
        drawTextured(mGround, groundWorld, view, proj, camPos, renderer);
    }

    if (mBallIndex >= 0) {
        auto& ball = mObjects[mBallIndex];
        float ballScale = mBall.getRadius() / mBall.getBaseRadius();
        DirectX::XMMATRIX world =
            DirectX::XMMatrixScaling(ballScale, ballScale, ballScale) *
            DirectX::XMMatrixRotationQuaternion(mBall.getOrientation()) *
            DirectX::XMMatrixTranslation(ball.position.x, ball.position.y, ball.position.z);
        for (auto& renderObj : ball.renderObjs) {
            drawTextured(renderObj, world, view, proj, camPos, renderer);
        }

        float outlineScale = mBall.getRadius() * 1.06f;
        DirectX::XMMATRIX outlineWorld =
            DirectX::XMMatrixScaling(outlineScale, outlineScale, outlineScale) *
            DirectX::XMMatrixRotationQuaternion(mBall.getOrientation()) *
            DirectX::XMMatrixTranslation(ball.position.x, ball.position.y, ball.position.z);
        drawTextured(mOutlineSphere, outlineWorld, view, proj, camPos, renderer);
    }

    DirectX::XMVECTOR ballPos = (mBallIndex >= 0)
        ? DirectX::XMLoadFloat3(&mObjects[mBallIndex].position)
        : DirectX::XMVectorSet(0, 0, 0, 0);
    DirectX::XMVECTOR ballOrientation = mBall.getOrientation();

    for (size_t i = 0; i < mObjects.size(); ++i) {
        if ((int)i == mBallIndex) continue;
        auto& obj = mObjects[i];
        float s = obj.scale * obj.scaleMultiplier;

        DirectX::XMMATRIX world;
        if (obj.isAttached) {
            DirectX::XMVECTOR wpos = computeObjectWorldPos(mObjects, (int)i, mBallIndex, ballPos, ballOrientation);
            DirectX::XMFLOAT3 wp;
            DirectX::XMStoreFloat3(&wp, wpos);
            DirectX::XMVECTOR wrot = computeObjectWorldRot(mObjects, (int)i, mBallIndex, ballOrientation);
            world =
                DirectX::XMMatrixScaling(s, s, s) *
                DirectX::XMMatrixRotationQuaternion(wrot) *
                DirectX::XMMatrixTranslation(wp.x, wp.y, wp.z);
        } else {
            world =
                DirectX::XMMatrixScaling(s, s, s) *
                DirectX::XMMatrixRotationX(obj.rotation.x) *
                DirectX::XMMatrixRotationY(obj.rotation.y) *
                DirectX::XMMatrixRotationZ(obj.rotation.z) *
                DirectX::XMMatrixTranslation(obj.position.x, obj.position.y, obj.position.z);
        }
        for (auto& renderObj : obj.renderObjs) {
            drawTextured(renderObj, world, view, proj, camPos, renderer);
        }
    }

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
                ImGui::Text("Camera Distance: %.1f", cam.getRadius());
                float maxSpeed = mBall.getMoveMaxSpeed();
                ImGui::SliderFloat("Move Speed", &maxSpeed, 5.0f, 100.0f, "%.1f");
                mBall.setMoveMaxSpeed(maxSpeed);
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
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Radius: %.2f", mBall.getRadius());
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
