#pragma once

#include "GameState.h"
#include "PongInputManager.h"
#include "SquareRenderObj.h"
#include <vector>

struct Projectile {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 velocity;
    bool active = false;
};

class PongState : public GameState {
public:
    explicit PongState(Game& game);
    void init() override;
    void onEnter() override;
    void handleInput(float deltaTime) override;
    void update(float deltaTime) override;
    void render(float deltaTime) override;

private:
    void resetBall();
    void updateTowers(float deltaTime);
    void updateProjectiles(float deltaTime);
    void fireProjectile(const DirectX::XMFLOAT3& fromPos,
                        const DirectX::XMFLOAT3& targetPos);

    Game& mGame;
    PongInputManager mInput;

    SquareRenderObj mLeftPaddle, mRightPaddle, mBall;
    SquareRenderObj mLeftTower, mRightTower;
    SquareRenderObj mProjectileRenderObj;
    std::vector<Projectile> mProjectiles;

    DirectX::XMFLOAT3 mLeftPaddlePos, mRightPaddlePos, mBallPos, mBallVel;
    DirectX::XMFLOAT3 mPaddleScale, mBallScale;
    DirectX::XMFLOAT3 mTowerScale, mBulletScale;
    int mScoreLeft = 0;
    int mScoreRight = 0;
    float mPaddleSpeed = 8.0f;
    float mTowerFireTimer = 0.0f;
    float mTowerFireInterval = 0.35f;
    float mTowerRange = 4.0f;
    float mBulletSpeed = 12.0f;
};
