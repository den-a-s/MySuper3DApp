#pragma once
#include <windows.h>

class KatamariInputManager {
public:
    void pumpMessages();

    bool isMoveForward() const;
    bool isMoveBackward() const;
    bool isMoveLeft() const;
    bool isMoveRight() const;
    bool isMoveUp() const;
    bool isMoveDown() const;
    bool isSwitchCamera() const;
    bool isExit() const;

    bool isMouseDown() const { return mMouseDown; }
    float getMouseDeltaX() const { return mMouseDeltaX; }
    float getMouseDeltaY() const { return mMouseDeltaY; }
    float getScrollDelta() const { return mScrollDelta; }
    bool isExitRequested() const { return mExitRequested; }

private:
    bool mMouseDown = false;
    int mMouseX = 0, mMouseY = 0;
    float mMouseDeltaX = 0, mMouseDeltaY = 0;
    float mScrollDelta = 0;
    bool mExitRequested = false;
};
