#pragma once

#include "SquareRenderObj.h"
#include "InputManager.h"
#include <chrono>
#include <vector>

struct Projectile {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 velocity;
    bool active = false;
};

class Game {
public:
    Game();
    void run();

private:
    void init();
    void handleInput(float deltaTime);
    void Update(float deltaTime);
    void Render(float deltaTime);
    void resetBall();
    void updateWindowTitle();
    void updateTowers(float deltaTime);
    void updateProjectiles(float deltaTime);
    void fireProjectile(const DirectX::XMFLOAT3& fromPos, const DirectX::XMFLOAT3& targetPos);

    Renderer mRenderer;
    InputManager mInputManager;
    SquareRenderObj mLeftPaddle, mRightPaddle, mBall;

    SquareRenderObj mLeftTower, mRightTower;
    SquareRenderObj mProjectileRenderObj;
    std::vector<Projectile> mProjectiles;

    DirectX::XMFLOAT3 mLeftPaddlePos, mRightPaddlePos, mBallPos, mBallVel;
    DirectX::XMFLOAT3 mPaddleScale, mBallScale;
    DirectX::XMFLOAT3 mTowerScale, mBulletScale;
    int mScoreLeft, mScoreRight;
    float mPaddleSpeed = 8.0f;
    float mTowerFireTimer = 0.0f;
    float mTowerFireInterval = 0.35f;
    float mTowerRange = 4.0f;
    float mBulletSpeed = 5.0f;
    bool mIsExitRequested = false;
};