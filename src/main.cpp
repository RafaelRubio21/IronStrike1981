#include "raylib.h"
#include "game/Game.h"

int main()
{
    const int screenWidth = 1024;
    const int screenHeight = 768;

    InitWindow(screenWidth, screenHeight, "Iron Strike 1981 - 2D Arcade");
    SetTargetFPS(60);

    Game game;
    game.Initialize();

    while (!WindowShouldClose())
    {
        game.Update(GetFrameTime());
        game.Render();
    }

    CloseWindow();
    return 0;
}
