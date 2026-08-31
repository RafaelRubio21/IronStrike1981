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
    
    // Inicia o Canvas Global de Sombras do jogo
    globalShadowTarget = LoadRenderTexture(1024, 768);

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
    // ETAPA 1: SHADOW PASS GLOBAL
    // Pede para todos os objetos do jogo desenharem suas sombras pretas solidas no Canvas
    BeginTextureMode(globalShadowTarget);
        ClearBackground(BLANK); // Limpa o fundo do buffer com alfa 0
        player.DrawShadows();
        // inimigo1.DrawShadows(); <-- Futuro
        // inimigo2.DrawShadows(); <-- Futuro
    EndTextureMode();

    // ETAPA 2: DESENHO PRINCIPAL NA TELA
    BeginDrawing();
    ClearBackground({ 34, 139, 34, 255 }); // Verde Floresta Escuro temporario
    
    // Inicia a Camera do Mapa (rola o fundo verticalmente)
    BeginMode2D(camera);
        // Quando criarmos as tilesets (chao e agua), desenhamos elas aqui embaixo!
    EndMode2D();

    // ETAPA 3: CARIMBA O CANVAS DE SOMBRAS UNIFICADO
    // Em OpenGL a textura renderizada fica de cabeca para baixo, por isso a altura do sourceRec é negativa!
    Rectangle sourceRec = { 0.0f, 0.0f, (float)globalShadowTarget.texture.width, -(float)globalShadowTarget.texture.height };
    Rectangle destRec = { 0.0f, 0.0f, (float)globalShadowTarget.texture.width, (float)globalShadowTarget.texture.height };
    // Aplica o nivel de transparencia 40 em todas as sombras unificadas!
    DrawTexturePro(globalShadowTarget.texture, sourceRec, destRec, { 0.0f, 0.0f }, 0.0f, { 255, 255, 255, 40 });

    // ETAPA 4: DESENHA AS CORES REAIS DOS OBJETOS POR CIMA DA SOMBRA
    player.DrawBody();
    // inimigo1.DrawBody(); <-- Futuro
    
    // UI DA TELA (Textos, HUD, FPS sempre desenhados por ultimo e FORA da câmera)
    DrawFPS(10, 10);

    EndDrawing();
}
