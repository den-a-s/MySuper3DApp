#pragma once
#include "GameState.h"

class Game;

class MenuState : public GameState {
public:
    explicit MenuState(Game& game);
    void init() override;
    void handleInput(float deltaTime) override;
    void update(float deltaTime) override;
    void render(float deltaTime) override;

private:
    Game& mGame;
};
