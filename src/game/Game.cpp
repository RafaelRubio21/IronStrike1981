#include "Game.h"
#include <raylib.h>

static Texture2D expFrames[10] = {0};
static bool expLoaded = false;
static Texture2D smokeFrames[7] = {0};
static bool smokeLoaded = false;

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
    explosions.clear();
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
    
    // Carrega os Sons de Impacto Metálico
    const char* audioPaths[] = {
        "assets/audio/metal_impact/",
        "../../assets/audio/metal_impact/",
        "../../../assets/audio/metal_impact/"
    };
    
    for (int i = 0; i < 5; i++)
    {
        impactSounds[i].frameCount = 0; // Zera para segurança
        for (int p = 0; p < 3; p++)
        {
            if (impactSounds[i].frameCount == 0)
            {
                impactSounds[i] = LoadSound(TextFormat("%simpact%d.ogg", audioPaths[p], i + 1));
            }
        }
    }
    
    // Carrega a Animação de Explosão (Explosion2_1 até Explosion2_10)
    if (!expLoaded)
    {
        const char* expPaths[] = {
            "assets/sprites/explosions/explosion2/",
            "../../assets/sprites/explosions/explosion2/",
            "../../../assets/sprites/explosions/explosion2/"
        };
        for (int p = 0; p < 3; p++)
        {
            if (expFrames[0].id == 0)
            {
                for (int f = 0; f < 10; f++)
                {
                    expFrames[f] = LoadTexture(TextFormat("%sExplosion2_%d.png", expPaths[p], f + 1));
                }
            }
        }
        expLoaded = true;
    }
    
    // Carrega a Animação de Fumaça (smoke1_1 até smoke1_7)
    if (!smokeLoaded)
    {
        const char* smokePaths[] = {
            "assets/sprites/smokes/smoke1/",
            "../../assets/sprites/smokes/smoke1/",
            "../../../assets/sprites/smokes/smoke1/"
        };
        for (int p = 0; p < 3; p++)
        {
            if (smokeFrames[0].id == 0)
            {
                for (int f = 0; f < 7; f++)
                {
                    smokeFrames[f] = LoadTexture(TextFormat("%ssmoke1_%d.png", smokePaths[p], f + 1));
                }
            }
        }
        smokeLoaded = true;
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
        tanks[i].Update(deltaTime, player.GetPosition());
        
        // Só tenta matar o tanque se ele já não estiver destruído
        if (!tanks[i].isDestroyed)
        {
            if (player.CheckBulletHits(tanks[i].GetHitbox()))
            {
                tanks[i].TakeDamage(1); // Arranca 1 de HP por bala
                
                // Se esse tiro acabou de destruir o tanque
                if (tanks[i].hp <= 0 && tanks[i].isDestroyed)
                {
                    Explosion ex;
                    ex.position = tanks[i].position;
                    ex.currentFrame = 0;
                    ex.frameTimer = 0.0f;
                    ex.scale = 1.0f; // Pode ajustar a escala da explosão
                    explosions.push_back(ex);
                }
                else
                {
                    // Escolhe 1 dos 5 sons de impacto aleatoriamente (só se não explodiu)
                    int randSfx = GetRandomValue(0, 4);
                    if (impactSounds[randSfx].frameCount != 0)
                    {
                        SetSoundVolume(impactSounds[randSfx], 0.7f);
                        PlaySound(impactSounds[randSfx]);
                    }
                }
            }
        }
        
        // Remove da lista se ele saiu da tela
        if (!tanks[i].isActive)
        {
            tanks.erase(tanks.begin() + i);
            i--;
        }
    }
    
    // Anima e remove explosões
    for (int i = 0; i < explosions.size(); i++)
    {
        explosions[i].frameTimer += deltaTime;
        if (explosions[i].frameTimer >= 0.05f) // 20 FPS
        {
            explosions[i].frameTimer = 0.0f;
            explosions[i].currentFrame++;
            if (explosions[i].currentFrame >= 10)
            {
                explosions.erase(explosions.begin() + i);
                i--;
            }
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
        
        // Desenha a sombra do player
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
    
    // ETAPA 5: DESENHAR FUMAÇAS E EXPLOSÕES POR CIMA DE TUDO
    for (const auto& t : tanks)
    {
        if (t.isDestroyed && smokeFrames[0].id != 0)
        {
            Texture2D sTex = smokeFrames[t.smokeFrame];
            float sW = (float)sTex.width;
            float sH = (float)sTex.height;
            Rectangle sSource = { 0.0f, 0.0f, (float)sTex.width, (float)sTex.height };
            // Fumaça deslocada um pouquinho pra cima e sempre sem rotação (0.0f)
            Rectangle sDest = { t.position.x, t.position.y - 10.0f, sW, sH };
            Vector2 sOrigin = { sW / 2.0f, sH / 2.0f };
            DrawTexturePro(sTex, sSource, sDest, sOrigin, 0.0f, WHITE);
        }
    }
    
    for (const auto& ex : explosions)
    {
        Texture2D tex = expFrames[ex.currentFrame];
        if (tex.id != 0)
        {
            float w = (float)tex.width * ex.scale;
            float h = (float)tex.height * ex.scale;
            Rectangle source = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
            Rectangle dest = { ex.position.x, ex.position.y, w, h };
            Vector2 origin = { w / 2.0f, h / 2.0f };
            DrawTexturePro(tex, source, dest, origin, 0.0f, WHITE);
        }
    }

    // UI DA TELA (Textos, HUD, FPS sempre desenhados por ultimo e FORA da câmera)
    DrawFPS(10, 10);

    EndDrawing();
}
