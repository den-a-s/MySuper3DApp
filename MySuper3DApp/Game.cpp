#include "Game.h"
#include "GameState.h"
#include "MenuState.h"
#include "PongState.h"
#include "SolarSystemState.h"
#include "KatamariAtHomeState.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

Game::Game() : mRenderer(Renderer::create()) {
    init();
}

Game::~Game() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Game::init() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(mRenderer.mDisplay->getHandlerWindow());
    ImGui_ImplDX11_Init(mRenderer.mDevice.Get(), mRenderer.mContext.Get());

    mCurrentState = std::make_unique<MenuState>(*this);
    mCurrentState->init();
    mCurrentState->onEnter();
}

void Game::run() {
    auto prevTime = std::chrono::steady_clock::now();

    while (!mIsExitRequested) {
        auto curTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(
                              curTime - prevTime).count() / 1000000.0f;
        prevTime = curTime;

        if (!mCurrentState) continue;

        mCurrentState->handleInput(deltaTime);
        mCurrentState->update(deltaTime);
        mCurrentState->render(deltaTime);

        if (mPendingState.has_value()) {
            auto type = mPendingState.value();
            mPendingState.reset();
            applySwitchState(type);
        }
    }
}

void Game::switchState(GameStateType type) {
    mPendingState = type;
}

void Game::applySwitchState(GameStateType type) {
    if (mCurrentState) {
        mCurrentState->onExit();
    }

    switch (type) {
        case GameStateType::Menu:
            mCurrentState = std::make_unique<MenuState>(*this);
            break;
        case GameStateType::Pong:
            mCurrentState = std::make_unique<PongState>(*this);
            break;
        case GameStateType::SolarSystem:
            mCurrentState = std::make_unique<SolarSystemState>(*this);
            break;
        case GameStateType::KatamariAtHome:
            mCurrentState = std::make_unique<KatamariAtHomeState>(*this);
            break;
    }

    mCurrentState->init();
    mCurrentState->onEnter();
}

void Game::setWindowSize(int width, int height) {
    if (mIsFullscreen) {
        setFullscreen(false);
    }
    mRenderer.mDisplay->resize(width, height);
    mRenderer.resize(width, height);
}

void Game::setFullscreen(bool fullscreen) {
    mIsFullscreen = fullscreen;
    mRenderer.mDisplay->setFullscreen(fullscreen);
    mRenderer.resize(mRenderer.mDisplay->getScreenWidth(),
                     mRenderer.mDisplay->getScreenHeight());
}
