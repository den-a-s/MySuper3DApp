#pragma once

#include "Renderer.h"
#include <chrono>
#include <memory>
#include <optional>

enum class GameStateType { Menu, Pong, SolarSystem, KatamariAtHome, KatamariGame };

class GameState;

class Game {
public:
    Game();
    ~Game();
    void run();

    void switchState(GameStateType type);
    void requestExit() { mIsExitRequested = true; }
    bool isExitRequested() const { return mIsExitRequested; }
    Renderer& getRenderer() { return mRenderer; }
    void setWindowSize(int width, int height);
    void setFullscreen(bool fullscreen);
    bool isFullscreen() const { return mIsFullscreen; }

private:
    void init();
    void applySwitchState(GameStateType type);

    Renderer mRenderer;
    std::unique_ptr<GameState> mCurrentState;
    std::optional<GameStateType> mPendingState;
    bool mIsExitRequested = false;
    bool mIsFullscreen = false;
};
