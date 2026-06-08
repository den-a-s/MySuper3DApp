#pragma once

#include "GameState.h"
#include "SolarInputManager.h"
#include "SquareRenderObj.h"
#include "CelestialBody.h"
#include <vector>
#include <string>

struct PlanetBody {
    PlanetData data;
    SquareRenderObj planetObj;
    SquareRenderObj moonObj;
    SquareRenderObj orbitObj;
    SquareRenderObj moonOrbitObj;
    bool planetIsBox = false;
    bool moonIsBox = false;
};

class SolarSystemState : public GameState {
public:
    explicit SolarSystemState(Game& game);
    void init() override;
    void onEnter() override;
    void handleInput(float deltaTime) override;
    void update(float deltaTime) override;
    void render(float deltaTime) override;

private:
    void reloadPlanets();

    Game& mGame;
    SolarInputManager mInput;

    SquareRenderObj mSunObj;
    std::vector<PlanetBody> mPlanets;

    std::string mPlanetsJsonPath = "../planets.json";
    std::string mReloadStatus;

    bool mMousePressed = false;
    int mMouseX = 0, mMouseY = 0;
    bool mCamSwitchHeld = false;
};
