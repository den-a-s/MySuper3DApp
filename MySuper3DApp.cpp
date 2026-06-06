#include "MySuper3DApp/Game.h"
#include <windows.h>
#include <exception>

int main() {
    try {
        Game game;
        game.run();
    } catch (const std::exception& e) {
        OutputDebugStringA("Error: ");
        OutputDebugStringA(e.what());
        OutputDebugStringA("\n");
        return -1;
    }
    return 0;
}