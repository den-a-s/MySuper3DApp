#pragma once
#include "GameState.h"
#include "KatamariInputManager.h"
#include "KatamariBall.h"
#include "TexturedRenderObj.h"
#include <vector>
#include <string>
#include <deque>
#include <random>

struct KatamariGameObject {
    std::vector<TexturedRenderObj> renderObjs;
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

class KatamariAtHomeState : public GameState {
public:
    explicit KatamariAtHomeState(Game& game);
    ~KatamariAtHomeState() override;
    void init() override;
    void onEnter() override;
    void handleInput(float deltaTime) override;
    void update(float deltaTime) override;
    void render(float deltaTime) override;

private:
    void loadScene();
    void scatterObjects();
    void log(const std::string& msg);

    Game& mGame;
    KatamariInputManager mInput;
    KatamariBall mBall;

    TexturedRenderObj mGround;
    std::vector<KatamariGameObject> mObjects;

    int mBallIndex = -1;

    float mCameraDistance = 15.0f;
    float mInitialAspect = 1.0f;
    std::string mScenePath = "../scene.json";
    std::string mSceneStatus;
    std::deque<std::string> mLog;
    std::mt19937 mRng;

    TexturedRenderObj mOutlineSphere;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> mOutlineRasterizerState;
};
