#include "Game.h"

void Game::Initialize()
{
    player.Initialize(Vector2{ 400.0f, 500.0f });

    // A camera 2D fica parada no 0,0 por enquanto
    camera = { 0 };
    camera.target = Vector2{ 0.0f, 0.0f };
    camera.offset = Vector2{ 0.0f, 0.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    levelScrollY = 0.0f;
    scrollSpeed = 100.0f; // Pixels por segundo
}

void Game::Update(float deltaTime)
{
    // Rola o cenario
    levelScrollY += scrollSpeed * deltaTime;

    player.Update(deltaTime);
}

void Game::Render()
{
    BeginDrawing();
    ClearBackground(DARKGREEN); // Cor provisoria da grama

    BeginMode2D(camera);

    // Desenhar elementos do mapa aqui (em breve)
    
    // Desenhar entidades
    player.Render();

    EndMode2D();

    DrawFPS(10, 10);
    EndDrawing();
}
