#pragma once
#include "GameState.h"
#include "KatamariInputManager.h"
#include "TexturedRenderObj.h"
#include <vector>
#include <string>
#include <deque>

struct KatamariObject {
    TexturedRenderObj renderObj;
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 rotation;
    float scale;
    float scaleMultiplier = 1.0f;
    std::string name;
    std::string objPath;
};

class KatamariAtHomeState : public GameState {
public:
    explicit KatamariAtHomeState(Game& game);
    void init() override;
    void onEnter() override;
    void handleInput(float deltaTime) override;
    void update(float deltaTime) override;
    void render(float deltaTime) override;

private:
    void loadScene();
    void saveScene();
    void resetCamera();
    DirectX::XMFLOAT4 getCameraPosition() const;
    void log(const std::string& msg);

    Game& mGame;
    KatamariInputManager mInput;

    TexturedRenderObj mGround;
    std::vector<KatamariObject> mObjects;

    float mMoveSpeed = 6.0f;
    float mInitialAspect = 1.0f;
    std::string mScenePath = "../scene.json";
    std::string mSceneStatus;
    std::deque<std::string> mLog;

    bool mMousePressed = false;
    int mMouseX = 0, mMouseY = 0;
    bool mCamSwitchHeld = false;
};
