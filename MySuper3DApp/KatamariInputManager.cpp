#include "KatamariInputManager.h"

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
