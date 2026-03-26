#include "Game.h"
#include <windows.h>
#include <algorithm>
#include <iostream>

Game::Game() : mRenderer(Renderer::create()) { 
    init(); 
}

void Game::run() {
    std::chrono::time_point<std::chrono::steady_clock> prevTime =
        std::chrono::steady_clock::now();
    unsigned int frameCount = 0;
    float totalTime = 0.0f;

    while (!mIsExitRequested) {
        handleInput();

        auto curTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(
                              curTime - prevTime)
                              .count() /
                          1000000.0f;
        prevTime = curTime;

        totalTime += deltaTime;
        frameCount++;
        if (totalTime > 1.0f) {
            totalTime -= 1.0f;
            frameCount = 0;
        }

        Update(deltaTime);
        Render(deltaTime);
    }
}

void Game::init() {
    mLeftPaddle =
        SquareRenderObj::create(mRenderer, L"./Shaders/MyVeryFirstShader.hlsl");
    mRightPaddle =
        SquareRenderObj::create(mRenderer, L"./Shaders/MyVeryFirstShader.hlsl");
    mBall =
        SquareRenderObj::create(mRenderer, L"./Shaders/MyVeryFirstShader.hlsl");

    mLeftPaddlePos = DirectX::XMFLOAT3(-3.5f, 0.0f, 0.0f);
    mRightPaddlePos = DirectX::XMFLOAT3(3.5f, 0.0f, 0.0f);
    mBallPos = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    mBallVel = DirectX::XMFLOAT3(2.0f, 1.5f, 0.0f);

    mPaddleScale = DirectX::XMFLOAT3(0.5f, 2.0f, 1.0f);
    mBallScale = DirectX::XMFLOAT3(0.4f, 0.4f, 1.0f);

    mScoreLeft = 0;
    mScoreRight = 0;

    updateWindowTitle();
}

void Game::handleInput() {
  MSG msg = {};
  while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  if (msg.message == WM_QUIT) {
    mIsExitRequested = true;
    return;
  }

    if (GetAsyncKeyState('W') & 0x8000) {
    mLeftPaddlePos.y += mPaddleSpeed * 0.016f;
  }
  if (GetAsyncKeyState('S') & 0x8000) {
    mLeftPaddlePos.y -= mPaddleSpeed * 0.016f;
  }
  mLeftPaddlePos.y = std::clamp(mLeftPaddlePos.y, -3.5f, 3.5f);

    if (GetAsyncKeyState(VK_UP) & 0x8000) {
    mRightPaddlePos.y += mPaddleSpeed * 0.016f;
  }
  if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
    mRightPaddlePos.y -= mPaddleSpeed * 0.016f;
  }
  mRightPaddlePos.y = std::clamp(mRightPaddlePos.y, -3.5f, 3.5f);
}

void Game::Update(float deltaTime) {

    mBallPos.x += mBallVel.x * deltaTime;
    mBallPos.y += mBallVel.y * deltaTime;

    DirectX::BoundingBox ballBox;
    ballBox.Center = DirectX::XMFLOAT3(mBallPos.x, mBallPos.y, 0.0f);
    ballBox.Extents =
        DirectX::XMFLOAT3(mBallScale.x * 0.5f, mBallScale.y * 0.5f, 0.5f);

    DirectX::BoundingBox topWall;
    topWall.Center = DirectX::XMFLOAT3(0.0f, 4.0f, 0.0f);
    topWall.Extents = DirectX::XMFLOAT3(5.0f, 0.1f, 1.0f);
    if (ballBox.Intersects(topWall)) {
        mBallVel.y = -mBallVel.y;
    }

    DirectX::BoundingBox bottomWall;
    bottomWall.Center = DirectX::XMFLOAT3(0.0f, -4.0f, 0.0f);
    bottomWall.Extents = DirectX::XMFLOAT3(5.0f, 0.1f, 1.0f);
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
    leftGoal.Center = DirectX::XMFLOAT3(-4.5f, 0.0f, 0.0f);
    leftGoal.Extents = DirectX::XMFLOAT3(0.2f, 4.5f, 1.0f);
    if (ballBox.Intersects(leftGoal)) {
        mScoreRight++;
        resetBall();
        updateWindowTitle();
    }

    DirectX::BoundingBox rightGoal;
    rightGoal.Center = DirectX::XMFLOAT3(4.5f, 0.0f, 0.0f);
    rightGoal.Extents = DirectX::XMFLOAT3(0.2f, 4.5f, 1.0f);
    if (ballBox.Intersects(rightGoal)) {
        mScoreLeft++;
        resetBall();
        updateWindowTitle();
    }
}

void Game::resetBall() {
    mBallPos = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    float angle = (rand() % 100 - 50) * 0.02f;
    mBallVel =
        DirectX::XMFLOAT3(2.0f * (mBallVel.x > 0 ? 1 : -1), 1.5f + angle, 0.0f);
}

void Game::updateWindowTitle() {
    WCHAR text[256];
    swprintf_s(text, L"Pong  %d  :  %d", mScoreLeft, mScoreRight);
    SetWindowText(mRenderer.mDisplay->getHandlerWindow(), text);
}

void Game::Render(float deltaTime) {
    float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
    mRenderer.beginFrame(clearColor);

    DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(
        DirectX::XMVectorSet(0.0f, 0.0f, -10.0f, 0.0f),
        DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
        DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    float aspect = static_cast<float>(mRenderer.mDisplay->getScreenWidth()) /
                   mRenderer.mDisplay->getScreenHeight();
    DirectX::XMMATRIX projection =
        DirectX::XMMatrixOrthographicOffCenterLH(-5.0f * aspect,  // лево
                                                 5.0f * aspect,   // право
                                                 -4.0f,           // низ
                                                 4.0f,            // верх
                                                 0.1f,     // ближн€€ плоскость
                                                 100.0f);  // дальн€€ плоскость

    DirectX::XMMATRIX worldLeft =
        DirectX::XMMatrixScaling(mPaddleScale.x, mPaddleScale.y,
                                 mPaddleScale.z) *
        DirectX::XMMatrixTranslation(mLeftPaddlePos.x, mLeftPaddlePos.y,
                                     mLeftPaddlePos.z);
    draw(mLeftPaddle, worldLeft, view, projection, mRenderer);

    DirectX::XMMATRIX worldRight =
        DirectX::XMMatrixScaling(mPaddleScale.x, mPaddleScale.y,
                                 mPaddleScale.z) *
        DirectX::XMMatrixTranslation(mRightPaddlePos.x, mRightPaddlePos.y,
                                     mRightPaddlePos.z);
    draw(mRightPaddle, worldRight, view, projection, mRenderer);

    DirectX::XMMATRIX worldBall =
        DirectX::XMMatrixScaling(mBallScale.x, mBallScale.y, mBallScale.z) *
        DirectX::XMMatrixTranslation(mBallPos.x, mBallPos.y, mBallPos.z);
    draw(mBall, worldBall, view, projection, mRenderer);

    mRenderer.endFrame();
}