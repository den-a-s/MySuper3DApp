#include "DisplayWin32.h"
#include "imgui.h"
#include <windows.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, umessage, wparam, lparam))
        return true;

    switch (umessage) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, umessage, wparam, lparam);
    }
}

DisplayWin32::DisplayWin32(const std::wstring& applicationName, const int screenHeight, const int screenWidth) {
    mApplicationName = applicationName;
    mHInstance = GetModuleHandle(nullptr);
    mWindowedRect = {};

    mWc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    mWc.lpfnWndProc = WndProc;
    mWc.cbClsExtra = 0;
    mWc.cbWndExtra = 0;
    mWc.hInstance = mHInstance;
    mWc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
    mWc.hIconSm = mWc.hIcon;
    mWc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    mWc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    mWc.lpszMenuName = nullptr;
    mWc.lpszClassName = mApplicationName.c_str();
    mWc.cbSize = sizeof(WNDCLASSEX);

    RegisterClassEx(&mWc);

    mScreenHeight = screenHeight;
    mScreenWidth = screenWidth;

    RECT windowRect = {0, 0, screenWidth, screenHeight};
    mWindowStyle = WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX;
    AdjustWindowRect(&windowRect, mWindowStyle, FALSE);

    auto posX = (GetSystemMetrics(SM_CXSCREEN) - screenWidth) / 2;
    auto posY = (GetSystemMetrics(SM_CYSCREEN) - screenHeight) / 2;

    mHWnd = CreateWindowEx(WS_EX_APPWINDOW, mApplicationName.c_str(),
                           mApplicationName.c_str(), mWindowStyle, posX, posY,
                           windowRect.right - windowRect.left,
                           windowRect.bottom - windowRect.top, nullptr, nullptr,
                           mHInstance, nullptr);

    ShowWindow(mHWnd, SW_SHOW);
    SetForegroundWindow(mHWnd);
    SetFocus(mHWnd);

    ShowCursor(true);
}

HWND DisplayWin32::getHandlerWindow() { 
    return mHWnd; 
}

int DisplayWin32::getScreenHeight() { 
    return mScreenHeight; 
}

int DisplayWin32::getScreenWidth() { 
    return mScreenWidth; 
}

void DisplayWin32::resize(int width, int height) {
    RECT rc = {0, 0, width, height};
    AdjustWindowRect(&rc, mWindowStyle, FALSE);

    auto posX = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    auto posY = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

    SetWindowPos(mHWnd, nullptr, posX, posY,
                 rc.right - rc.left,
                 rc.bottom - rc.top,
                 SWP_NOZORDER);

    mScreenWidth = width;
    mScreenHeight = height;
}

void DisplayWin32::setFullscreen(bool fullscreen) {
    if (fullscreen) {
        GetWindowRect(mHWnd, &mWindowedRect);

        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);

        SetWindowLongPtr(mHWnd, GWL_STYLE, WS_POPUP);
        SetWindowPos(mHWnd, nullptr, 0, 0, screenW, screenH,
                     SWP_FRAMECHANGED | SWP_NOZORDER);

        mScreenWidth = screenW;
        mScreenHeight = screenH;
    } else {
        SetWindowLongPtr(mHWnd, GWL_STYLE, mWindowStyle);
        SetWindowPos(mHWnd, nullptr,
                     mWindowedRect.left, mWindowedRect.top,
                     mWindowedRect.right - mWindowedRect.left,
                     mWindowedRect.bottom - mWindowedRect.top,
                     SWP_FRAMECHANGED | SWP_NOZORDER);

        mScreenWidth = mWindowedRect.right - mWindowedRect.left;
        mScreenHeight = mWindowedRect.bottom - mWindowedRect.top;
    }
}

bool DisplayWin32::isFullscreen() const {
    return (GetWindowLongPtr(mHWnd, GWL_STYLE) & WS_POPUP) != 0;
}