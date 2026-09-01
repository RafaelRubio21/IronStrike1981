#include "raylib.h"
#include "game/AssetPath.h"
#include "game/Constants.h"
#include "game/Game.h"

int main()
{
    // Entra no diretório que contém a pasta "assets", para que todo
    // carregamento use um caminho único, sem cascata de "../../"
    if (!LocateAssetsRoot())
    {
        TraceLog(LOG_WARNING, "Pasta 'assets' nao encontrada: o jogo vai rodar sem sprites nem sons.");
    }

    InitWindow(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, "Iron Strike 1981 - 2D Arcade");
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
