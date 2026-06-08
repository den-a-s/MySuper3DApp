#pragma once
#include "GameState.h"
#include "KatamariInputManager.h"
#include "TexturedRenderObj.h"
#include <vector>
#include <string>
#include <deque>
#include <random>

struct KatamariGameObject {
    TexturedRenderObj renderObj;
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 rotation;
    float scale;
    float scaleMultiplier = 1.0f;
    float gameSize = 0.0f;
    std::string name;
    std::string objPath;

    float boundingRadius = 0.0f;
    bool isAttached = false;
    DirectX::XMFLOAT3 attachOffset = {0, 0, 0};
    int parentIndex = -1;
    bool isBall = false;
};

class KatamariGameState : public GameState {
public:
    explicit KatamariGameState(Game& game);
    void init() override;
    void onEnter() override;
    void handleInput(float deltaTime) override;
    void update(float deltaTime) override;
    void render(float deltaTime) override;

private:
    void loadScene();
    void scatterObjects();
    DirectX::XMVECTOR computeObjectWorldPos(int index, DirectX::XMVECTOR ballPos) const;
    void updateBallSize(float absorbedSize);
    void updateBall(float dt);
    void checkPickups();
    void log(const std::string& msg);

    Game& mGame;
    KatamariInputManager mInput;

    TexturedRenderObj mGround;
    std::vector<KatamariGameObject> mObjects;

    int mBallIndex = -1;
    float mBallRadius = 1.0f;
    float mBallBaseRadius = 1.0f;
    DirectX::XMVECTOR mBallOrientation;

    DirectX::XMFLOAT4 mSavedRot;
    DirectX::XMFLOAT3 mBallVelocity;
    float mMoveDrag = 5.0f;
    float mRotationDrag = 0.14f;
    float mRotationMaxSpeed = 0.1f;
    float mMoveMaxSpeed = 20.0f;

    float mCameraDistance = 15.0f;
    float mInitialAspect = 1.0f;
    std::string mScenePath = "../scene.json";
    std::string mSceneStatus;
    std::deque<std::string> mLog;
    std::mt19937 mRng;

    bool mMousePressed = false;
    int mMouseX = 0, mMouseY = 0;

    TexturedRenderObj mOutlineSphere;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> mOutlineRasterizerState;
};
