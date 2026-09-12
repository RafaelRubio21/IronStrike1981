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

    // Centraliza no monitor principal (índice 0). Sem isso, com mais de um
    // monitor conectado o SO às vezes abre a janela num monitor secundário,
    // ou fora de centro nele.
    const int monitorPrincipal = 0;
    const Vector2 monitorPos = GetMonitorPosition(monitorPrincipal);
    const int monitorWidth = GetMonitorWidth(monitorPrincipal);
    const int monitorHeight = GetMonitorHeight(monitorPrincipal);
    SetWindowPosition(
        (int)monitorPos.x + (monitorWidth - Config::SCREEN_WIDTH) / 2,
        (int)monitorPos.y + (monitorHeight - Config::SCREEN_HEIGHT) / 2);

    InitAudioDevice(); // Inicia o Motor de Som (Hardware)
    SetTargetFPS(60);

    // O Game vive num escopo próprio: ele precisa liberar texturas e sons
    // ENQUANTO a janela e o dispositivo de áudio ainda existem.
    {
        Game game;
        game.Initialize();

        while (!WindowShouldClose())
        {
            // Um travamento (janela arrastada, disco lento, troca de app)
            // devolve um deltaTime enorme. Como TODO movimento aqui é v*dt,
            // um quadro desses teleportaria os tanques para fora da rota e
            // faria a bala pular por cima do alvo. Preso em 20 FPS: se o jogo
            // engasgar, ele fica lento em vez de dar salto.
            float deltaTime = GetFrameTime();
            if (deltaTime > Config::MAX_DELTA_TIME) deltaTime = Config::MAX_DELTA_TIME;

            game.Update(deltaTime);
            game.Render();
        }

        game.Shutdown();
    }

    CloseAudioDevice(); // Desliga a placa de som
    CloseWindow();
    return 0;
}
