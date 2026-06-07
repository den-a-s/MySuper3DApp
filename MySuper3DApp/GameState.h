#pragma once

class Game;

class GameState {
public:
    virtual ~GameState() = default;
    virtual void init() = 0;
    virtual void handleInput(float deltaTime) = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render(float deltaTime) = 0;
    virtual void onEnter() {}
    virtual void onExit() {}
};
