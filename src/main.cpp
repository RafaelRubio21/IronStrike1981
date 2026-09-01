#include "raylib.h"
#include "game/Game.h"

int main()
{
    const int screenWidth = 1024;
    const int screenHeight = 768;

    InitWindow(screenWidth, screenHeight, "Iron Strike 1981 - 2D Arcade");
    InitAudioDevice(); // Inicia o Motor de Som (Hardware)
    SetTargetFPS(60);

    // O Game vive num escopo próprio: ele precisa liberar texturas e sons
    // ENQUANTO a janela e o dispositivo de áudio ainda existem.
    {
        Game game;
        game.Initialize();

        while (!WindowShouldClose())
        {
            game.Update(GetFrameTime());
            game.Render();
        }

        game.Shutdown();
    }

    CloseAudioDevice(); // Desliga a placa de som
    CloseWindow();
    return 0;
}
