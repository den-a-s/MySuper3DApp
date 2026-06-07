#include "MenuState.h"
#include "Game.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <windows.h>

MenuState::MenuState(Game& game) : mGame(game) {}

void MenuState::init() {}

void MenuState::handleInput(float deltaTime) {
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    if (msg.message == WM_QUIT) {
        mGame.requestExit();
        return;
    }
}

void MenuState::update(float deltaTime) {}

void MenuState::render(float deltaTime) {
    float clearColor[] = {0.05f, 0.05f, 0.1f, 1.0f};
    auto& renderer = mGame.getRenderer();
    renderer.beginFrame(clearColor);

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImVec2 windowSize(320, 280);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
    ImGui::SetNextWindowPos(
        ImVec2((renderer.mDisplay->getScreenWidth() - windowSize.x) * 0.5f,
               (renderer.mDisplay->getScreenHeight() - windowSize.y) * 0.5f),
        ImGuiCond_Always);

    ImGui::Begin("Main Menu", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse);

    ImGui::SetWindowFontScale(1.2f);
    ImGui::Spacing();

    ImGui::SetWindowFontScale(1.5f);
    float bw = ImGui::GetContentRegionAvail().x;

    if (ImGui::Button("Pong", ImVec2(bw, 60))) {
        mGame.switchState(GameStateType::Pong);
    }

    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::Button("Solar System", ImVec2(bw, 60))) {
        mGame.switchState(GameStateType::SolarSystem);
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    renderer.endFrame();
}
