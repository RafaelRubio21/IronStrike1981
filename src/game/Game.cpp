#include "Game.h"

void Game::Initialize()
{
    // A camera 2D fica parada no 0,0 por enquanto
    camera = { 0 };
    camera.target = Vector2{ 0.0f, 0.0f };
    camera.offset = Vector2{ 0.0f, 0.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    player.Initialize({ 1024.0f / 2.0f, 600.0f });

    tanks.clear();
    tankSpawnTimer = 0.0f;

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
    
    // Gerador de Tanques
    tankSpawnTimer += deltaTime;
    if (tankSpawnTimer >= 2.0f) // Cria um tanque a cada 2 segundos
    {
        tankSpawnTimer = 0.0f;
        int type = GetRandomValue(0, 2);
        
        Tank t;
        if (type == 0) // Vem de cima
            t.Initialize({ (float)GetRandomValue(100, 924), -100.0f }, 0);
        else if (type == 1) // Vem da esquerda
            t.Initialize({ -100.0f, (float)GetRandomValue(100, 600) }, 1);
        else // Vem da direita
            t.Initialize({ 1124.0f, (float)GetRandomValue(100, 600) }, 2);
            
        tanks.push_back(t);
    }
    
    // Atualiza os tanques, checa colisão com tiro, e remove os inativos
    for (int i = 0; i < tanks.size(); i++)
    {
        tanks[i].Update(deltaTime);
        
        // Só tenta matar o tanque se ele já não estiver destruído
        if (!tanks[i].isDestroyed)
        {
            if (player.CheckBulletHits(tanks[i].GetHitbox()))
            {
                tanks[i].Destroy(); // Destrói o tanque! (Toca animação/som no futuro)
            }
        }
        
        // Remove da lista se ele saiu da tela
        if (!tanks[i].isActive)
        {
            tanks.erase(tanks.begin() + i);
            i--;
        }
    }
}

void Game::Render()
{
    // ETAPA 1: SHADOW PASS GLOBAL
    BeginTextureMode(globalShadowTarget);
        ClearBackground(BLANK); // Limpa o fundo do buffer com alfa 0
        
        // Desenha a sombra de todos os tanques primeiro
        for (const auto& t : tanks) t.DrawShadows();
        
        // Desenha a sombra do player (pra sobrepor os inimigos terrestres se estiver voando)
        player.DrawShadows();
    EndTextureMode();

    // ETAPA 2: DESENHO PRINCIPAL NA TELA
    BeginDrawing();
    ClearBackground({ 34, 139, 34, 255 }); // Verde Floresta Escuro temporario
    
    BeginMode2D(camera);
        // Quando criarmos as tilesets (chao e agua), desenhamos elas aqui embaixo!
    EndMode2D();

    // ETAPA 3: CARIMBA O CANVAS DE SOMBRAS UNIFICADO
    Rectangle sourceRec = { 0.0f, 0.0f, (float)globalShadowTarget.texture.width, -(float)globalShadowTarget.texture.height };
    Rectangle destRec = { 0.0f, 0.0f, (float)globalShadowTarget.texture.width, (float)globalShadowTarget.texture.height };
    // Aplica o nivel de transparencia 40 em todas as sombras unificadas!
    DrawTexturePro(globalShadowTarget.texture, sourceRec, destRec, { 0.0f, 0.0f }, 0.0f, { 255, 255, 255, 40 });

    // ETAPA 4: DESENHA AS CORES REAIS DOS OBJETOS POR CIMA DA SOMBRA
    for (const auto& t : tanks) t.DrawBody();
    player.DrawBody();
    
    // UI DA TELA (Textos, HUD, FPS sempre desenhados por ultimo e FORA da câmera)
    DrawFPS(10, 10);

    EndDrawing();
}
