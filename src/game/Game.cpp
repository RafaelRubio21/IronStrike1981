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

    // Carrega a Música de Fundo
    bgMusicVolume = 0.0f; // Comeca 100% mudo para o Fade-In
    bgMusicTargetVolume = 0.4f; // Volume alvo onde ela deve estabilizar

    bgMusic = LoadMusicStream("assets/audio/bgmusic/music1.ogg");
    if (bgMusic.frameCount == 0) bgMusic = LoadMusicStream("../../assets/audio/bgmusic/music1.ogg");
    if (bgMusic.frameCount == 0) bgMusic = LoadMusicStream("../../../assets/audio/bgmusic/music1.ogg");
    
    if (bgMusic.frameCount != 0) 
    {
        bgMusic.looping = true;
        SetMusicVolume(bgMusic, bgMusicVolume);
        PlayMusicStream(bgMusic);
    }
}

void Game::Update(float deltaTime)
{
    // Atualiza a musica de fundo e faz o Fade-In
    if (bgMusic.frameCount != 0) 
    {
        UpdateMusicStream(bgMusic);

        // Se o volume atual for menor que o alvo, vai subindo devagar!
        if (bgMusicVolume < bgMusicTargetVolume)
        {
            bgMusicVolume += 0.05f * deltaTime; // Velocidade do Fade In
            if (bgMusicVolume > bgMusicTargetVolume) bgMusicVolume = bgMusicTargetVolume;
            
            SetMusicVolume(bgMusic, bgMusicVolume);
        }
    }

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
