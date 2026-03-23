#include "DisplayWin32.h"
#include <windows.h>

LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam) {
    switch (umessage) {
        case WM_KEYDOWN:
            if (static_cast<unsigned int>(wparam) == 27) {
                PostQuitMessage(0);
            }
            return 0;
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
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    auto dwStyle = WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_THICKFRAME;

    auto posX = (GetSystemMetrics(SM_CXSCREEN) - screenWidth) / 2;
    auto posY = (GetSystemMetrics(SM_CYSCREEN) - screenHeight) / 2;

    mHWnd = CreateWindowEx(WS_EX_APPWINDOW, mApplicationName.c_str(),
                           mApplicationName.c_str(), dwStyle, posX, posY,
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