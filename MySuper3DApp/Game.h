#pragma once

#include "SquareRenderObj.h"
#include "InputManager.h"
#include "CelestialBody.h"
#include <chrono>
#include <vector>

struct PlanetBody {
    PlanetData data;
    SquareRenderObj planetObj;
    SquareRenderObj moonObj;
    SquareRenderObj orbitObj;
    bool planetIsBox;
    bool moonIsBox;
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

    Renderer mRenderer;
    InputManager mInputManager;

    SquareRenderObj mSunObj;
    std::vector<PlanetBody> mPlanets;

    bool mMousePressed = false;
    int mMouseX = 0, mMouseY = 0;
    bool mCamSwitchHeld = false;
    bool mProjSwitchHeld = false;

    WCHAR mWindowTitle[128];

    bool mIsExitRequested = false;
};
