#pragma once
#include <windows.h>

class InputManager {
public:
    bool isLeftPaddleUp() const;
    bool isLeftPaddleDown() const;
    bool isRightPaddleUp() const;
    bool isRightPaddleDown() const;
    bool isExit() const;
};
