#include "InputManager.h"

bool InputManager::isLeftPaddleUp() const {
    return GetAsyncKeyState(0x57) & 0x8000;
}

bool InputManager::isLeftPaddleDown() const {
    return GetAsyncKeyState(0x53) & 0x8000;
}

bool InputManager::isRightPaddleUp() const {
    return GetAsyncKeyState(VK_UP) & 0x8000;
}

bool InputManager::isRightPaddleDown() const {
    return GetAsyncKeyState(VK_DOWN) & 0x8000;
}

bool InputManager::isExit() const {
    return GetAsyncKeyState(VK_ESCAPE) & 0x8000;
}
