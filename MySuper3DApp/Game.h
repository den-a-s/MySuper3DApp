#pragma once

#include "SquareRenderObj.h"
#include "InputManager.h"
#include "CelestialBody.h"
#include <chrono>
#include <vector>
#include <string>

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
    ~Game();
    void run();

private:
    void init();
    void reloadPlanets();
    void handleInput(float deltaTime);
    void Update(float deltaTime);
    void Render(float deltaTime);

    Renderer mRenderer;
    InputManager mInputManager;

    SquareRenderObj mSunObj;
    std::vector<PlanetBody> mPlanets;

    std::string mPlanetsJsonPath = "../planets.json";
    std::string mReloadStatus;

    bool mMousePressed = false;
    int mMouseX = 0, mMouseY = 0;
    bool mCamSwitchHeld = false;

    bool mIsExitRequested = false;
};
