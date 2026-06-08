#include "KatamariInputManager.h"

void KatamariInputManager::pumpMessages() {
    mMouseDeltaX = 0;
    mMouseDeltaY = 0;
    mScrollDelta = 0;

    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        switch (msg.message) {
        case WM_MOUSEMOVE: {
            int x = static_cast<int>(LOWORD(msg.lParam));
            int y = static_cast<int>(HIWORD(msg.lParam));
            if (mMouseDown) {
                mMouseDeltaX += static_cast<float>(x - mMouseX) * 0.005f;
                mMouseDeltaY += static_cast<float>(mMouseY - y) * 0.005f;
            }
            mMouseX = x;
            mMouseY = y;
            break;
        }
        case WM_LBUTTONDOWN:
            mMouseDown = true;
            mMouseX = static_cast<int>(LOWORD(msg.lParam));
            mMouseY = static_cast<int>(HIWORD(msg.lParam));
            break;
        case WM_LBUTTONUP:
            mMouseDown = false;
            break;
        case WM_MOUSEWHEEL:
            mScrollDelta += -GET_WHEEL_DELTA_WPARAM(msg.wParam) / 120.0f * 5.0f;
            break;
        case WM_QUIT:
            mExitRequested = true;
            break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

bool KatamariInputManager::isMoveForward() const {
    return GetAsyncKeyState('W') & 0x8000;
}
bool KatamariInputManager::isMoveBackward() const {
    return GetAsyncKeyState('S') & 0x8000;
}
bool KatamariInputManager::isMoveLeft() const {
    return GetAsyncKeyState('A') & 0x8000;
}
bool KatamariInputManager::isMoveRight() const {
    return GetAsyncKeyState('D') & 0x8000;
}
bool KatamariInputManager::isMoveUp() const {
    return GetAsyncKeyState(VK_SPACE) & 0x8000;
}
bool KatamariInputManager::isMoveDown() const {
    return GetAsyncKeyState(VK_SHIFT) & 0x8000;
}
bool KatamariInputManager::isSwitchCamera() const {
    return GetAsyncKeyState('C') & 0x8000;
}
bool KatamariInputManager::isExit() const {
    return GetAsyncKeyState(VK_ESCAPE) & 0x8000;
}
