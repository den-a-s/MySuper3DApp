#pragma once

#include <windows.h>
#include <string>

class DisplayWin32 {
public:
    DisplayWin32(const std::wstring& applicationName, const int screenHeight, const int screenWidth);
    
    HWND getHandlerWindow();
    int getScreenHeight();
    int getScreenWidth();

private:
    std::wstring mApplicationName;
    HINSTANCE mHInstance;
    WNDCLASSEX mWc;
    LONG mScreenWidth;
    LONG mScreenHeight;
    HWND mHWnd;
};

LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam);