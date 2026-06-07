#include "MenuState.h"
#include "Game.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <windows.h>

static const int sSizePresets[][2] = {
    {800, 600},
    {1024, 768},
    {1280, 720},
    {1920, 1080},
};
static const int sPresetCount = sizeof(sSizePresets) / sizeof(sSizePresets[0]);
static const char* sSizeNames[] = {
    "800x600",
    "1024x768",
    "1280x720",
    "1920x1080",
};

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

    ImVec2 windowSize(340, 380);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
    ImGui::SetNextWindowPos(
        ImVec2((renderer.mDisplay->getScreenWidth() - windowSize.x) * 0.5f,
               (renderer.mDisplay->getScreenHeight() - windowSize.y) * 0.5f),
        ImGuiCond_Always);

    ImGui::Begin("Main Menu", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse);

    ImGui::SetWindowFontScale(1.5f);
    float bw = ImGui::GetContentRegionAvail().x;

    if (ImGui::Button("Pong", ImVec2(bw, 60))) {
        mGame.switchState(GameStateType::Pong);
    }

    ImGui::Spacing();

    if (ImGui::Button("Solar System", ImVec2(bw, 60))) {
        mGame.switchState(GameStateType::SolarSystem);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::SetWindowFontScale(1.2f);
    ImGui::Text("Window");
    ImGui::Spacing();

    int curW = renderer.mDisplay->getScreenWidth();
    int curH = renderer.mDisplay->getScreenHeight();
    int selected = 0;
    for (int i = 0; i < sPresetCount; i++) {
        if (curW == sSizePresets[i][0] && curH == sSizePresets[i][1]) {
            selected = i;
            break;
        }
    }

    ImGui::SetWindowFontScale(1.0f);
    ImGui::Text("Resolution");
    ImGui::SameLine();
    if (ImGui::BeginCombo("##res", sSizeNames[selected])) {
        for (int i = 0; i < sPresetCount; i++) {
            bool isSelected = (i == selected);
            if (ImGui::Selectable(sSizeNames[i], isSelected)) {
                mGame.setWindowSize(sSizePresets[i][0], sSizePresets[i][1]);
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    renderer.endFrame();
}
