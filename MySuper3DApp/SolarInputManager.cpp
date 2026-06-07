#include "SolarInputManager.h"

bool SolarInputManager::isMoveForward() const {
    return GetAsyncKeyState(0x57) & 0x8000; // W
}

bool SolarInputManager::isMoveBackward() const {
    return GetAsyncKeyState(0x53) & 0x8000; // S
}

bool SolarInputManager::isMoveLeft() const {
    return GetAsyncKeyState(0x41) & 0x8000; // A
}

bool SolarInputManager::isMoveRight() const {
    return GetAsyncKeyState(0x44) & 0x8000; // D
}

bool SolarInputManager::isMoveUp() const {
    return GetAsyncKeyState(VK_SPACE) & 0x8000; // Space
}

bool SolarInputManager::isMoveDown() const {
    return GetAsyncKeyState(VK_SHIFT) & 0x8000; // Shift
}

bool SolarInputManager::isSwitchCamera() const {
    return GetAsyncKeyState(0x43) & 0x8000; // C
}

bool SolarInputManager::isExit() const {
    return GetAsyncKeyState(VK_ESCAPE) & 0x8000;
}
