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

KatamariGameState::KatamariGameState(Game& game) : mGame(game), mRng(std::random_device{}()) {
    mBallOrientation = DirectX::XMQuaternionIdentity();
    DirectX::XMStoreFloat4(&mSavedRot, DirectX::XMQuaternionIdentity());
    mBallVelocity = {0, 0, 0};
}

void KatamariGameState::init() {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

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
        mBallBaseRadius = mObjects[mBallIndex].boundingRadius;
        mBallRadius = mBallBaseRadius;
        mCameraDistance = max(mCameraDistance, mBallRadius * 3.0f);
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
        renderer.mDevice->CreateRasterizerState(&outlineRastDesc, mOutlineRasterizerState.GetAddressOf());
        mOutlineSphere.mRasterizerState = mOutlineRasterizerState;
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

void KatamariGameState::scatterObjects() {
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
    DirectX::XMStoreFloat4(&mSavedRot, DirectX::XMQuaternionIdentity());
    mBallVelocity = {0, 0, 0};
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
            cam.zoomOrbit(-delta / 120.0f * 5.0f);
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

void KatamariGameState::updateBallSize(float absorbedSize) {
    float tmp = sqrtf(mBallRadius * mBallRadius + absorbedSize * absorbedSize);
    mBallRadius = tmp;
    mRotationMaxSpeed = 0.15f / (tmp * tmp);
    if (mRotationMaxSpeed < 0.005f)
        mRotationMaxSpeed = 0.005f;
    mMoveMaxSpeed = 20.0f * sqrtf(tmp);
    mRotationDrag = 0.1f + 0.06f / sqrtf(tmp);
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

    // Compute move direction from input (camera-relative)
    DirectX::XMVECTOR moveDir = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    if (mInput.isMoveForward())  moveDir = DirectX::XMVectorAdd(moveDir, forward);
    if (mInput.isMoveBackward()) moveDir = DirectX::XMVectorSubtract(moveDir, forward);
    if (mInput.isMoveRight())    moveDir = DirectX::XMVectorAdd(moveDir, right);
    if (mInput.isMoveLeft())     moveDir = DirectX::XMVectorSubtract(moveDir, right);

    // SetDirection equivalent: build rolling rotation + set velocity
    float moveLen = DirectX::XMVectorGetX(DirectX::XMVector3Length(moveDir));
    if (moveLen > 0.001f) {
        DirectX::XMVECTOR tmp = DirectX::XMVectorSet(
            DirectX::XMVectorGetX(moveDir), 0.0f,
            DirectX::XMVectorGetZ(moveDir), 0.0f);
        tmp = DirectX::XMVector3Normalize(tmp);

        // Rolling quaternion: axis = cross(tmp, Up)
        DirectX::XMVECTOR rollAxis = DirectX::XMVector3Cross(tmp,
            DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
        float axisLen = DirectX::XMVectorGetX(DirectX::XMVector3Length(rollAxis));
        if (axisLen > 0.0001f) {
            rollAxis = DirectX::XMVectorScale(rollAxis, 1.0f / axisLen);
            DirectX::XMVECTOR q = DirectX::XMQuaternionRotationAxis(rollAxis, -mRotationMaxSpeed);

            // f = angle(savedRot, Identity) / 0.1f — rolling speed limiter
            DirectX::XMVECTOR s = DirectX::XMLoadFloat4(&mSavedRot);
            float w = fmaxf(-1.0f, fminf(1.0f, DirectX::XMVectorGetW(s)));
            float angle = 2.0f * acosf(w);
            float f = fminf(1.0f, angle / 0.1f);
            DirectX::XMVECTOR lerped = DirectX::XMQuaternionSlerp(q, DirectX::XMQuaternionIdentity(), f);
            DirectX::XMVECTOR newSavedRot = DirectX::XMQuaternionMultiply(s, lerped);
            newSavedRot = DirectX::XMQuaternionNormalize(newSavedRot);
            DirectX::XMStoreFloat4(&mSavedRot, newSavedRot);
        }

        // Set velocity
        DirectX::XMStoreFloat3(&mBallVelocity,
            DirectX::XMVectorScale(tmp, mMoveMaxSpeed));
    }

    // savedRot decays toward Identity (RotateTowards equivalent)
    {
        DirectX::XMVECTOR s = DirectX::XMLoadFloat4(&mSavedRot);
        float w = fmaxf(-1.0f, fminf(1.0f, DirectX::XMVectorGetW(s)));
        float angle = 2.0f * acosf(w);
        float step = mRotationDrag * dt;
        if (angle > step && angle > 0.0001f) {
            float t = step / angle;
            s = DirectX::XMQuaternionSlerp(s, DirectX::XMQuaternionIdentity(), t);
        } else {
            s = DirectX::XMQuaternionIdentity();
        }
        DirectX::XMStoreFloat4(&mSavedRot, s);
    }

    // Apply savedRot to ball orientation (right-multiply = local space, like reference)
    mBallOrientation = DirectX::XMQuaternionMultiply(
        mBallOrientation, DirectX::XMLoadFloat4(&mSavedRot));
    mBallOrientation = DirectX::XMQuaternionNormalize(mBallOrientation);

    // Apply velocity drag
    float dragFactor = fmaxf(0.0f, 1.0f - mMoveDrag * dt);
    mBallVelocity.x *= dragFactor;
    mBallVelocity.z *= dragFactor;

    // Integrate position
    ball.position.x += mBallVelocity.x * dt;
    ball.position.z += mBallVelocity.z * dt;

    ball.position.y = mBallRadius;
}

void KatamariGameState::checkPickups() {
    if (mBallIndex < 0) return;
    DirectX::XMVECTOR ballPos = DirectX::XMLoadFloat3(&mObjects[mBallIndex].position);

    // Pass 1: ball picks up objects
    for (int i = 0; i < (int)mObjects.size(); ++i) {
        if (i == mBallIndex || mObjects[i].isAttached) continue;
        auto& obj = mObjects[i];

        DirectX::XMVECTOR objPos = DirectX::XMLoadFloat3(&obj.position);
        DirectX::XMVECTOR diff = DirectX::XMVectorSubtract(objPos, ballPos);
        float dist = DirectX::XMVectorGetX(DirectX::XMVector3Length(diff));
        float objR = obj.boundingRadius * obj.scale * obj.scaleMultiplier;
        float sumRadii = mBallRadius + objR;

        if (dist < sumRadii && mBallRadius > obj.gameSize) {
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
            updateBallSize(obj.gameSize);
            log("Picked up " + obj.name);
        }
    }

    // Pass 2: chain pickups — objects attach to other attached objects
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

            if (dist < freeR + attachedR && mBallRadius > freeObj.gameSize) {
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
                updateBallSize(freeObj.gameSize);
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
        for (auto& renderObj : ball.renderObjs) {
            drawTextured(renderObj, world, view, proj, camPos, renderer);
        }

        // Wireframe outline sphere
        float outlineScale = mBallRadius * 1.06f;
        DirectX::XMMATRIX outlineWorld =
            DirectX::XMMatrixScaling(outlineScale, outlineScale, outlineScale) *
            DirectX::XMMatrixRotationQuaternion(mBallOrientation) *
            DirectX::XMMatrixTranslation(ball.position.x, ball.position.y, ball.position.z);
        drawTextured(mOutlineSphere, outlineWorld, view, proj, camPos, renderer);
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
        for (auto& renderObj : obj.renderObjs) {
            drawTextured(renderObj, world, view, proj, camPos, renderer);
        }
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
                ImGui::Text("Camera Distance: %.1f", cam.getRadius());
                ImGui::SliderFloat("Move Speed", &mMoveMaxSpeed, 5.0f, 100.0f, "%.1f");
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
