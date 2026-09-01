#include "Game.h"
#include <raylib.h>
#include <cmath>


void Game::Initialize()
{
    // A camera 2D fica parada no 0,0 por enquanto
    camera = { 0 };
    camera.target = Vector2{ 0.0f, 0.0f };
    camera.offset = Vector2{ 0.0f, 0.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    player.Initialize({ 1024.0f / 2.0f, 600.0f });

    enemies.clear();
    explosionManager.Clear();
    enemyBullets.clear();
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
    
    explosionManager.Initialize();
    smokeManager.Initialize();
    
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
    
    // Se o player acabou de bater no chão após ser destruído, spawnar a explosão!
    if (player.justHitGround)
    {
        player.justHitGround = false;
        // Explosão 3 é a do player, um pouco maior
        explosionManager.Spawn(player.GetPosition(), ExplosionType::TYPE_1, 1.5f);
        
        // Se tivermos um som de explosão de player, tocaríamos aqui
    }
    
    // Gerador de Tanques
    tankSpawnTimer += deltaTime;
    if (tankSpawnTimer >= 2.0f) // Cria um tanque a cada 2 segundos
    {
        tankSpawnTimer = 0.0f;
        int type = GetRandomValue(0, 2);
        
        auto t = std::make_unique<Tank>();
        if (type == 0) // Vem de cima
            t->Initialize({ (float)GetRandomValue(100, 924), -100.0f }, 0);
        else if (type == 1) // Vem da esquerda
            t->Initialize({ -100.0f, (float)GetRandomValue(100, 600) }, 1);
        else // Vem da direita
            t->Initialize({ 1124.0f, (float)GetRandomValue(100, 600) }, 2);
            
        enemies.push_back(std::move(t));
    }
    
    // Atualiza os tanques, checa colisão com tiro, e remove os inativos
    for (int i = 0; i < enemies.size(); i++)
    {
        enemies[i]->Update(deltaTime, player.GetPosition(), player.isDestroyed);
        
        // Se o tanque atirou neste frame, instanciamos a bala inimiga!
        if (enemies[i]->hasFired)
        {
            enemies[i]->hasFired = false;
            
            // O Tank usa rotação 0 = para BAIXO (Y+)
            float rad = enemies[i]->cannonRotation * DEG2RAD;
            Vector2 forward = { -sinf(rad), cosf(rad) };
            
            // Spawna na ponta do cano
            float offset = std::abs(enemies[i]->fireOffsetY);
            Vector2 startPos = { enemies[i]->position.x + (forward.x * offset), enemies[i]->position.y + (forward.y * offset) };
            EnemyBullet bullet;
            bullet.Initialize(startPos, forward, 250.0f);
            enemyBullets.push_back(bullet);
        }
        
        if (enemies[i]->isDestroyed)
        {
            enemies[i]->hitTimer -= deltaTime;
        }
        
        // Só tenta matar o tanque se ele já não estiver destruído
        if (!enemies[i]->isDestroyed)
        {
            if (player.CheckBulletHits(enemies[i]->GetHitbox()))
            {
                enemies[i]->TakeDamage(1); // Arranca 1 de HP por bala
                
                // Se esse tiro acabou de destruir o tanque
                if (enemies[i]->hp <= 0 && enemies[i]->isDestroyed)
                {
                    explosionManager.Spawn(enemies[i]->position, ExplosionType::TYPE_0, 1.0f);
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
        if (!enemies[i]->isActive)
        {
            enemies.erase(enemies.begin() + i);
            i--;
        }
    }
    
    // Anima e remove explosões
    explosionManager.Update(deltaTime);
    
    // Atualiza Balas Inimigas
    for (int i = 0; i < enemyBullets.size(); i++)
    {
        auto& b = enemyBullets[i];
        
        if (b.active)
        {
            b.position.x += b.velocity.x * deltaTime;
            b.position.y += b.velocity.y * deltaTime;
            
            // Criação do Rastro (Fumaça)
            b.trailTimer += deltaTime;
            if (b.trailTimer >= 0.03f) // A cada 0.03s deixa um rastro
            {
                b.trailTimer = 0.0f;
                b.trail.push_back(b.position);
                if (b.trail.size() > 10) b.trail.erase(b.trail.begin());
            }
            
            // Colisão com o Jogador
            if (!player.isDestroyed && CheckCollisionPointRec(b.position, player.GetHitbox()))
            {
                player.TakeDamage(20); // Bala de tanque arranca 20 de vida
                b.active = false;
                
                // Explode na carcaça do player
                explosionManager.Spawn(b.position, ExplosionType::TYPE_0, 0.5f);
            }
        }
        else
        {
            // Bala já explodiu, dissipar a fumaça
            b.trailTimer += deltaTime;
            if (b.trailTimer >= 0.03f && !b.trail.empty())
            {
                b.trailTimer = 0.0f;
                b.trail.erase(b.trail.begin());
            }
        }
        
        // Destrói se sair muito da tela ou se já dissipou toda a fumaça após bater
        if (b.position.x < -200 || b.position.x > 1200 || b.position.y < -200 || b.position.y > 1000 || (!b.active && b.trail.empty()))
        {
            enemyBullets.erase(enemyBullets.begin() + i);
            i--;
        }
    }
}

void Game::Render()
{
    // ETAPA 1: DESENHO PRINCIPAL NA TELA
    BeginDrawing();
    ClearBackground({ 34, 139, 34, 255 }); // Verde Floresta Escuro temporario

    // ETAPA 2: SHADOW PASS GLOBAL
    BeginTextureMode(globalShadowTarget);
        ClearBackground(BLANK); // Limpa o fundo do buffer com alfa 0
        
        // Desenha a sombra de todos os inimigos primeiro
        for (const auto& e : enemies) e->DrawShadows();
        
        // Desenha a sombra do player
        player.DrawShadows();
    EndTextureMode();
    
    BeginMode2D(camera);
        // Quando criarmos as tilesets (chao e agua), desenhamos elas aqui embaixo!
        for (const auto& e : enemies) e->DrawGroundEffects();
    EndMode2D();

    // ETAPA 3: CARIMBA O CANVAS DE SOMBRAS UNIFICADO
    Rectangle sourceRec = { 0.0f, 0.0f, (float)globalShadowTarget.texture.width, -(float)globalShadowTarget.texture.height };
    Rectangle destRec = { 0.0f, 0.0f, (float)globalShadowTarget.texture.width, (float)globalShadowTarget.texture.height };
    // Aplica o nivel de transparencia nas sombras unificadas!
    DrawTexturePro(globalShadowTarget.texture, sourceRec, destRec, { 0.0f, 0.0f }, 0.0f, { 255, 255, 255, 80 });

    // ETAPA 4: DESENHA AS CORES REAIS DOS OBJETOS POR CIMA DA SOMBRA
    for (const auto& e : enemies) e->DrawBody();
    player.DrawBody();
    
    // ETAPA 5: DESENHAR FUMAÇAS E EXPLOSÕES POR CIMA DE TUDO
    for (const auto& e : enemies)
    {
        if (e->isDestroyed) smokeManager.Render(e->position, e->smokeFrame);
    }
    
    // Tiros Inimigos
    for (const auto& b : enemyBullets)
    {
        b.Render();
    }
    
    explosionManager.Render();

    // UI
    DrawFPS(10, 10);

    EndDrawing();


}

