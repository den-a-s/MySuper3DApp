#pragma once
#include <windows.h>

class KatamariInputManager {
public:
    bool isMoveForward() const;
    bool isMoveBackward() const;
    bool isMoveLeft() const;
    bool isMoveRight() const;
    bool isMoveUp() const;
    bool isMoveDown() const;
    bool isSwitchCamera() const;
    bool isExit() const;
};
