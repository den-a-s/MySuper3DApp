#include "InputManager.h"

bool InputManager::isMoveForward() const {
    return GetAsyncKeyState(0x57) & 0x8000; // W
}

bool InputManager::isMoveBackward() const {
    return GetAsyncKeyState(0x53) & 0x8000; // S
}

bool InputManager::isMoveLeft() const {
    return GetAsyncKeyState(0x41) & 0x8000; // A
}

bool InputManager::isMoveRight() const {
    return GetAsyncKeyState(0x44) & 0x8000; // D
}

bool InputManager::isMoveUp() const {
    return GetAsyncKeyState(VK_SPACE) & 0x8000; // Space
}

bool InputManager::isMoveDown() const {
    return GetAsyncKeyState(VK_SHIFT) & 0x8000; // Shift
}

bool InputManager::isSwitchCamera() const {
    return GetAsyncKeyState(0x43) & 0x8000; // C
}

bool InputManager::isExit() const {
    return GetAsyncKeyState(VK_ESCAPE) & 0x8000;
}
