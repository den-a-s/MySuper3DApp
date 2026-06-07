#include "PongInputManager.h"

bool PongInputManager::isLeftPaddleUp() const {
    return GetAsyncKeyState(0x57) & 0x8000;
}

bool PongInputManager::isLeftPaddleDown() const {
    return GetAsyncKeyState(0x53) & 0x8000;
}

bool PongInputManager::isRightPaddleUp() const {
    return GetAsyncKeyState(VK_UP) & 0x8000;
}

bool PongInputManager::isRightPaddleDown() const {
    return GetAsyncKeyState(VK_DOWN) & 0x8000;
}

bool PongInputManager::isExit() const {
    return GetAsyncKeyState(VK_ESCAPE) & 0x8000;
}
