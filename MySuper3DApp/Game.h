#pragma once

#include "SquareRenderObj.h"
#include <chrono>

class Game {
public:
    Game();
    void run();

private:
    void init();
    void handleInput();
    void Update(float deltaTime);
    void Render(float deltaTime);
    void resetBall();
    void updateWindowTitle();

    Renderer mRenderer;
    SquareRenderObj mLeftPaddle, mRightPaddle, mBall;

    DirectX::XMFLOAT3 mLeftPaddlePos, mRightPaddlePos, mBallPos, mBallVel;
    DirectX::XMFLOAT3 mPaddleScale, mBallScale;
    int mScoreLeft, mScoreRight;
    float mPaddleSpeed = 8.0f;
    bool mIsExitRequested = false;
};