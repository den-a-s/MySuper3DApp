#include "PongState.h"
#include "Game.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <windows.h>
#include <cstdio>
#include <algorithm>

PongState::PongState(Game& game) : mGame(game) {}

void PongState::init() {
    auto white = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    auto paddleMesh = createColoredQuadMeshData(white);
    auto& renderer = mGame.getRenderer();
    mLeftPaddle = SquareRenderObj::create(renderer, L"../Shaders/MyVeryFirstShader.hlsl", paddleMesh);
    mRightPaddle = SquareRenderObj::create(renderer, L"../Shaders/MyVeryFirstShader.hlsl", paddleMesh);
    mBall = SquareRenderObj::create(renderer, L"../Shaders/MyVeryFirstShader.hlsl", paddleMesh);

    auto brown = DirectX::XMFLOAT4(0.55f, 0.27f, 0.07f, 1.0f);
    auto towerMesh = createColoredQuadMeshData(brown);
    mLeftTower = SquareRenderObj::create(renderer, L"../Shaders/MyVeryFirstShader.hlsl", towerMesh);
    mRightTower = SquareRenderObj::create(renderer, L"../Shaders/MyVeryFirstShader.hlsl", towerMesh);

    auto yellow = DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
    auto bulletMesh = createColoredQuadMeshData(yellow);
    mProjectileRenderObj = SquareRenderObj::create(renderer, L"../Shaders/MyVeryFirstShader.hlsl", bulletMesh);

    mLeftPaddlePos = DirectX::XMFLOAT3(-3.5f, 0.0f, 0.0f);
    mRightPaddlePos = DirectX::XMFLOAT3(3.5f, 0.0f, 0.0f);
    mBallPos = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    mBallVel = DirectX::XMFLOAT3(2.0f, 1.5f, 0.0f);

    mPaddleScale = DirectX::XMFLOAT3(0.5f, 2.0f, 1.0f);
    mBallScale = DirectX::XMFLOAT3(0.4f, 0.4f, 1.0f);
    mTowerScale = DirectX::XMFLOAT3(0.3f, 0.3f, 1.0f);
    mBulletScale = DirectX::XMFLOAT3(0.15f, 0.15f, 1.0f);

    mScoreLeft = 0;
    mScoreRight = 0;

    mProjectiles.reserve(100);
}

void PongState::onEnter() {
    auto& renderer = mGame.getRenderer();
    float aspect = (float)renderer.mDisplay->getScreenWidth() /
                   (float)renderer.mDisplay->getScreenHeight();
    mFieldEdgeX = 4.5f * aspect;
    mLeftPaddlePos.x = -3.5f * aspect;
    mRightPaddlePos.x = 3.5f * aspect;
    auto& cam = renderer.getCamera();
    cam.initOrtho(4.5f, 4.0f, aspect);
}

void PongState::handleInput(float deltaTime) {
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
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

    if (mInput.isLeftPaddleUp()) {
        mLeftPaddlePos.y += mPaddleSpeed * deltaTime;
    }
    if (mInput.isLeftPaddleDown()) {
        mLeftPaddlePos.y -= mPaddleSpeed * deltaTime;
    }
    float paddleClampY = 4.0f - mPaddleScale.y * 0.5f;
    mLeftPaddlePos.y = std::clamp(mLeftPaddlePos.y, -paddleClampY, paddleClampY);

    if (mInput.isRightPaddleUp()) {
        mRightPaddlePos.y += mPaddleSpeed * deltaTime;
    }
    if (mInput.isRightPaddleDown()) {
        mRightPaddlePos.y -= mPaddleSpeed * deltaTime;
    }
    mRightPaddlePos.y = std::clamp(mRightPaddlePos.y, -paddleClampY, paddleClampY);
}

void PongState::update(float deltaTime) {
    mBallPos.x += mBallVel.x * deltaTime;
    mBallPos.y += mBallVel.y * deltaTime;

    DirectX::BoundingBox ballBox;
    ballBox.Center = DirectX::XMFLOAT3(mBallPos.x, mBallPos.y, 0.0f);
    ballBox.Extents =
        DirectX::XMFLOAT3(mBallScale.x * 0.5f, mBallScale.y * 0.5f, 0.5f);

    DirectX::BoundingBox topWall;
    topWall.Center = DirectX::XMFLOAT3(0.0f, 4.0f, 0.0f);
    topWall.Extents = DirectX::XMFLOAT3(mFieldEdgeX, 0.1f, 1.0f);
    if (ballBox.Intersects(topWall)) {
        mBallVel.y = -mBallVel.y;
    }

    DirectX::BoundingBox bottomWall;
    bottomWall.Center = DirectX::XMFLOAT3(0.0f, -4.0f, 0.0f);
    bottomWall.Extents = DirectX::XMFLOAT3(mFieldEdgeX, 0.1f, 1.0f);
    if (ballBox.Intersects(bottomWall)) {
        mBallVel.y = -mBallVel.y;
    }

    DirectX::BoundingBox leftPaddleBox;
    leftPaddleBox.Center =
        DirectX::XMFLOAT3(mLeftPaddlePos.x, mLeftPaddlePos.y, 0.0f);
    leftPaddleBox.Extents =
        DirectX::XMFLOAT3(mPaddleScale.x * 0.5f, mPaddleScale.y * 0.5f, 0.5f);

    if (ballBox.Intersects(leftPaddleBox)) {
        mBallVel.x = -mBallVel.x;
        mBallVel.y += (mBallPos.y - mLeftPaddlePos.y) * 0.5f;
        mBallPos.x = mLeftPaddlePos.x + leftPaddleBox.Extents.x +
                     ballBox.Extents.x + 0.01f;
    }

    DirectX::BoundingBox rightPaddleBox;
    rightPaddleBox.Center =
        DirectX::XMFLOAT3(mRightPaddlePos.x, mRightPaddlePos.y, 0.0f);
    rightPaddleBox.Extents =
        DirectX::XMFLOAT3(mPaddleScale.x * 0.5f, mPaddleScale.y * 0.5f, 0.5f);

    if (ballBox.Intersects(rightPaddleBox)) {
        mBallVel.x = -mBallVel.x;
        mBallVel.y += (mBallPos.y - mRightPaddlePos.y) * 0.5f;
        mBallPos.x = mRightPaddlePos.x -
                     (rightPaddleBox.Extents.x + ballBox.Extents.x + 0.01f);
    }

    DirectX::BoundingBox leftGoal;
    leftGoal.Center = DirectX::XMFLOAT3(-mFieldEdgeX, 0.0f, 0.0f);
    leftGoal.Extents = DirectX::XMFLOAT3(0.2f, 4.0f, 1.0f);
    if (ballBox.Intersects(leftGoal)) {
        mScoreRight++;
        resetBall();
    }

    DirectX::BoundingBox rightGoal;
    rightGoal.Center = DirectX::XMFLOAT3(mFieldEdgeX, 0.0f, 0.0f);
    rightGoal.Extents = DirectX::XMFLOAT3(0.2f, 4.0f, 1.0f);
    if (ballBox.Intersects(rightGoal)) {
        mScoreLeft++;
        resetBall();
    }

    updateTowers(deltaTime);
    updateProjectiles(deltaTime);
}

void PongState::resetBall() {
    mBallPos = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    float angle = (rand() % 100 - 50) * 0.02f;
    int dir = (rand() % 2 == 0) ? 1 : -1;
    mBallVel =
        DirectX::XMFLOAT3(2.0f * dir, 1.5f + angle, 0.0f);
}



void PongState::updateTowers(float deltaTime) {
    mTowerFireTimer += deltaTime;
    if (mTowerFireTimer >= mTowerFireInterval) {
        mTowerFireTimer -= mTowerFireInterval;

        float towerYOffset = mPaddleScale.y * 0.5f + mTowerScale.y * 0.5f;
        DirectX::XMFLOAT3 leftTowerPos = {
            mLeftPaddlePos.x, mLeftPaddlePos.y + towerYOffset, 0.0f};
        DirectX::XMFLOAT3 rightTowerPos = {
            mRightPaddlePos.x, mRightPaddlePos.y + towerYOffset, 0.0f};

        float leftDist = mBallPos.x - mLeftPaddlePos.x;
        if (leftDist > 0.0f && leftDist < mTowerRange) {
            fireProjectile(leftTowerPos, mBallPos);
        }

        float rightDist = mRightPaddlePos.x - mBallPos.x;
        if (rightDist > 0.0f && rightDist < mTowerRange) {
            fireProjectile(rightTowerPos, mBallPos);
        }
    }
}

void PongState::updateProjectiles(float deltaTime) {
    DirectX::BoundingBox ballBox;
    ballBox.Center = DirectX::XMFLOAT3(mBallPos.x, mBallPos.y, 0.0f);
    ballBox.Extents =
        DirectX::XMFLOAT3(mBallScale.x * 0.5f, mBallScale.y * 0.5f, 0.5f);

    for (auto& p : mProjectiles) {
        if (!p.active) continue;

        p.position.x += p.velocity.x * deltaTime;
        p.position.y += p.velocity.y * deltaTime;

        DirectX::BoundingBox bulletBox;
        bulletBox.Center =
            DirectX::XMFLOAT3(p.position.x, p.position.y, 0.0f);
        bulletBox.Extents =
            DirectX::XMFLOAT3(mBulletScale.x * 0.5f, mBulletScale.y * 0.5f, 0.5f);

        if (ballBox.Intersects(bulletBox)) {
            mBallVel.x += p.velocity.x * 0.3f;
            mBallVel.y += p.velocity.y * 0.3f;
            p.active = false;
        }

        if (p.position.x < -mFieldEdgeX || p.position.x > mFieldEdgeX ||
            p.position.y < -4.0f || p.position.y > 4.0f) {
            p.active = false;
        }
    }
}

void PongState::fireProjectile(const DirectX::XMFLOAT3& fromPos,
                               const DirectX::XMFLOAT3& targetPos) {
    Projectile* p = nullptr;
    for (auto& proj : mProjectiles) {
        if (!proj.active) {
            p = &proj;
            break;
        }
    }
    if (!p) {
        mProjectiles.emplace_back();
        p = &mProjectiles.back();
    }

    DirectX::XMVECTOR from = DirectX::XMLoadFloat3(&fromPos);
    DirectX::XMVECTOR target = DirectX::XMLoadFloat3(&targetPos);
    DirectX::XMVECTOR dir = DirectX::XMVectorSubtract(target, from);
    float len;
    DirectX::XMStoreFloat(&len, DirectX::XMVector3Length(dir));
    if (len < 0.001f) {
        dir = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    } else {
        dir = DirectX::XMVectorScale(dir, 1.0f / len);
    }

    DirectX::XMFLOAT3 vel;
    DirectX::XMStoreFloat3(&vel, DirectX::XMVectorScale(dir, mBulletSpeed));

    p->position = DirectX::XMFLOAT3(fromPos.x, fromPos.y, 0.0f);
    p->velocity = vel;
    p->active = true;
}

void PongState::render(float deltaTime) {
    auto& renderer = mGame.getRenderer();
    float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
    renderer.beginFrame(clearColor);

    draw(mLeftPaddle, mLeftPaddlePos, mPaddleScale, renderer);
    draw(mRightPaddle, mRightPaddlePos, mPaddleScale, renderer);
    draw(mBall, mBallPos, mBallScale, renderer);

    {
        DirectX::XMFLOAT3 towerOffset =
            DirectX::XMFLOAT3(0.0f, mPaddleScale.y * 0.5f + mTowerScale.y * 0.5f, 0.0f);
        DirectX::XMFLOAT3 leftTowerPos = {
            mLeftPaddlePos.x, mLeftPaddlePos.y + towerOffset.y, 0.0f};
        DirectX::XMFLOAT3 rightTowerPos = {
            mRightPaddlePos.x, mRightPaddlePos.y + towerOffset.y, 0.0f};
        draw(mLeftTower, leftTowerPos, mTowerScale, renderer);
        draw(mRightTower, rightTowerPos, mTowerScale, renderer);
    }

    for (auto& p : mProjectiles) {
        if (p.active) {
            draw(mProjectileRenderObj, p.position, mBulletScale, renderer);
        }
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit")) {
                    mGame.switchState(GameStateType::Menu);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                ImGui::TextUnformatted("Left Paddle:  W / S");
                ImGui::TextUnformatted("Right Paddle: Up / Down");
                ImGui::Separator();
                ImGui::TextUnformatted("ESC - Back to Menu");
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    {
        ImGui::Begin("Pong", nullptr,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground |
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
        ImGui::SetWindowPos(ImVec2(10, 24), ImGuiCond_Always);
        ImGui::SetWindowFontScale(1.8f);
        ImGui::TextColored(ImVec4(1, 1, 1, 1), "%d  :  %d", mScoreLeft, mScoreRight);
        ImGui::End();
    }

    {
        ImGui::Begin("FPS", nullptr,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground |
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
        ImGui::SetWindowPos(ImVec2(10, 60), ImGuiCond_Always);
        ImGui::SetWindowFontScale(1.0f);
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "FPS: %.0f", 1.0f / deltaTime);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    renderer.endFrame();
}
