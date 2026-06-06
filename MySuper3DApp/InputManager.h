#pragma once
#include <windows.h>

class InputManager {
public:
    bool isMoveForward() const;
    bool isMoveBackward() const;
    bool isMoveLeft() const;
    bool isMoveRight() const;
    bool isMoveUp() const;
    bool isMoveDown() const;
    bool isSwitchCamera() const;
    bool isSwitchProjection() const;
    bool isExit() const;
};
