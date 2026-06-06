#include "Game.h"
#include <windows.h>
#include <cstdio>
#include <algorithm>

static PlanetData makePlanet(float orbitRadius, float orbitSpeed, float selfRotSpeed, float size,
                              DirectX::XMFLOAT4 color,
                              float moonOrbitRadius, float moonOrbitSpeed, float moonSelfRotSpeed, float moonSize,
                              DirectX::XMFLOAT4 moonColor) {
    PlanetData p;
    p.orbitRadius = orbitRadius;
    p.orbitSpeed = orbitSpeed;
    p.angle = 0.0f;
    p.rotationSpeed = selfRotSpeed;
    p.rotationAngle = 0.0f;
    p.size = size;
    p.color = color;
    p.moon.orbitRadius = moonOrbitRadius;
    p.moon.orbitSpeed = moonOrbitSpeed;
    p.moon.angle = 0.0f;
    p.moon.rotationSpeed = moonSelfRotSpeed;
    p.moon.rotationAngle = 0.0f;
    p.moon.size = moonSize;
    p.moon.color = moonColor;
    return p;
}

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

    auto white = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);

    auto planetDefs = std::vector<PlanetData>{
        makePlanet(1.8f,  0.80f, 2.5f, 0.12f, DirectX::XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f),   0.25f, 1.5f, 5.0f, 0.04f, DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f)),
        makePlanet(2.6f,  0.55f, 1.8f, 0.18f, DirectX::XMFLOAT4(0.9f, 0.8f, 0.3f, 1.0f),   0.30f, 1.2f, 4.5f, 0.05f, DirectX::XMFLOAT4(0.7f, 0.6f, 0.2f, 1.0f)),
        makePlanet(3.5f,  0.40f, 3.0f, 0.22f, DirectX::XMFLOAT4(0.2f, 0.5f, 0.9f, 1.0f),   0.35f, 2.0f, 6.0f, 0.06f, DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f)),
        makePlanet(4.4f,  0.35f, 2.2f, 0.16f, DirectX::XMFLOAT4(0.8f, 0.3f, 0.1f, 1.0f),   0.28f, 1.8f, 5.5f, 0.04f, DirectX::XMFLOAT4(0.5f, 0.3f, 0.1f, 1.0f)),
        makePlanet(5.8f,  0.20f, 4.0f, 0.50f, DirectX::XMFLOAT4(0.8f, 0.6f, 0.2f, 1.0f),   0.55f, 1.0f, 6.5f, 0.10f, DirectX::XMFLOAT4(0.6f, 0.4f, 0.1f, 1.0f)),
        makePlanet(7.5f,  0.15f, 3.5f, 0.40f, DirectX::XMFLOAT4(0.9f, 0.7f, 0.1f, 1.0f),   0.50f, 0.8f, 5.0f, 0.09f, DirectX::XMFLOAT4(0.7f, 0.5f, 0.1f, 1.0f)),
        makePlanet(9.0f,  0.10f, 2.8f, 0.28f, DirectX::XMFLOAT4(0.3f, 0.8f, 0.8f, 1.0f),   0.40f, 0.7f, 6.0f, 0.06f, DirectX::XMFLOAT4(0.2f, 0.6f, 0.6f, 1.0f)),
        makePlanet(10.5f, 0.07f, 2.0f, 0.26f, DirectX::XMFLOAT4(0.1f, 0.2f, 0.8f, 1.0f),   0.38f, 0.6f, 5.0f, 0.05f, DirectX::XMFLOAT4(0.1f, 0.3f, 0.6f, 1.0f)),
        makePlanet(12.0f, 0.05f, 1.5f, 0.10f, DirectX::XMFLOAT4(0.8f, 0.7f, 0.5f, 1.0f),   0.20f, 0.5f, 4.0f, 0.03f, DirectX::XMFLOAT4(0.6f, 0.5f, 0.3f, 1.0f)),
    };

    mPlanets.reserve(9);
    for (auto& def : planetDefs) {
        PlanetBody body;
        body.data = def;

        auto planetMesh = createSphereMeshData(def.color, 1.0f, 16, 12);
        body.planetObj = SquareRenderObj::create(mRenderer, L"../Shaders/MyVeryFirstShader.hlsl", planetMesh);

        auto moonMesh = createSphereMeshData(def.moon.color, 1.0f, 12, 8);
        body.moonObj = SquareRenderObj::create(mRenderer, L"../Shaders/MyVeryFirstShader.hlsl", moonMesh);

        auto ringMesh = createRingMeshData(white, 0.98f, 1.02f, 48);
        body.orbitObj = SquareRenderObj::create(mRenderer, L"../Shaders/MyVeryFirstShader.hlsl", ringMesh);

        mPlanets.push_back(std::move(body));
    }

    mRenderer.initCamera(1.0f);

    SetWindowText(mRenderer.mDisplay->getHandlerWindow(), L"Planet Simulation");
}

void Game::handleInput(float deltaTime) {
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        switch (msg.message) {
        case WM_LBUTTONDOWN:
            mMousePressed = true;
            mMouseX = static_cast<int>(LOWORD(msg.lParam));
            mMouseY = static_cast<int>(HIWORD(msg.lParam));
            break;
        case WM_LBUTTONUP:
            mMousePressed = false;
            break;
        case WM_MOUSEMOVE:
            if (mMousePressed) {
                int x = static_cast<int>(LOWORD(msg.lParam));
                int y = static_cast<int>(HIWORD(msg.lParam));
                float dx = static_cast<float>(x - mMouseX) * 0.005f;
                float dy = static_cast<float>(mMouseY - y) * 0.005f;
                mRenderer.getCamera().rotate(dx, dy);
                mMouseX = x;
                mMouseY = y;
            }
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

    float moveSpeed = 6.0f * deltaTime;

    float forward = 0.0f, right = 0.0f, up = 0.0f;
    if (mInputManager.isMoveForward())  forward += moveSpeed;
    if (mInputManager.isMoveBackward()) forward -= moveSpeed;
    if (mInputManager.isMoveRight())    right += moveSpeed;
    if (mInputManager.isMoveLeft())     right -= moveSpeed;
    if (mInputManager.isMoveUp())       up += moveSpeed;
    if (mInputManager.isMoveDown())     up -= moveSpeed;

    if (forward != 0.0f || right != 0.0f || up != 0.0f) {
        mRenderer.getCamera().move(forward, right, up);
    }

    mRenderer.getCamera().update();
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

    mRenderer.endFrame();
}
