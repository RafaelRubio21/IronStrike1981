#include "Game.h"
#include "Constants.h"
#include <raylib.h>
#include <algorithm>
#include <cmath>


void Game::Initialize()
{
    // A camera 2D fica parada no 0,0 por enquanto
    camera = { 0 };
    camera.target = Vector2{ 0.0f, 0.0f };
    camera.offset = Vector2{ 0.0f, 0.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    player.Initialize({ Config::SCREEN_WIDTH / 2.0f, 600.0f });

    enemies.clear();
    explosionManager.Clear();
    enemyBullets.clear();
    tankSpawnTimer = 0.0f;

    mapManager.Load("assets/maps/level1.json");

    scrollSpeed = 10.0f; // Pixels por segundo
    
    // Inicia o Canvas Global de Sombras do jogo
    globalShadowTarget = LoadRenderTexture(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);

    // Carrega a Música de Fundo
    bgMusicVolume = 0.0f; // Comeca 100% mudo para o Fade-In
    bgMusicTargetVolume = 0.4f; // Volume alvo onde ela deve estabilizar

    bgMusic = LoadMusicStream("assets/audio/bgmusic/music1.ogg");
    
    if (bgMusic.frameCount != 0) 
    {
        bgMusic.looping = true;
        SetMusicVolume(bgMusic, bgMusicVolume);
        PlayMusicStream(bgMusic);
    }
    
    // Carrega os Sons de Impacto Metálico
    for (int i = 0; i < IMPACT_SOUND_COUNT; i++)
    {
        impactSounds[i] = LoadSound(TextFormat("assets/audio/metal_impact/impact%d.ogg", i + 1));
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
    mapManager.Update(deltaTime, scrollSpeed);

    player.Update(deltaTime);
    
    // Se o player acabou de bater no chão após ser destruído, spawnar a explosão!
    if (player.justHitGround)
    {
        player.justHitGround = false;
        // Explosão 3 é a do player, um pouco maior
        explosionManager.Spawn(player.GetPosition(), ExplosionType::TYPE_1, 1.5f);
        
        // Se tivermos um som de explosão de player, tocaríamos aqui
    }
    
    // Spawn de Inimigos via Mapa do Tiled
    std::vector<EnemySpawnData> newSpawns = mapManager.PopReadySpawns();
    for (const auto& spawn : newSpawns)
    {
        if (spawn.type == "Tank" || spawn.type == "EnemyRoute" || spawn.type == "EnemyPatrol")
        {
            int spawnCount = (spawn.type == "EnemyPatrol") ? spawn.quantity : 1;
            if (spawnCount <= 0) spawnCount = 1;

            for (int i = 0; i < spawnCount; i++) 
            {
                float screenX = spawn.x;
                float screenY = spawn.y - mapManager.GetScrollY();
                Rectangle patrolArea = {0,0,0,0};

                if (spawn.type == "EnemyPatrol") {
                    // Sorteia posição dentro do retângulo
                    screenX = spawn.x + (float)GetRandomValue(0, (int)spawn.width);
                    screenY = (spawn.y + (float)GetRandomValue(0, (int)spawn.height)) - mapManager.GetScrollY();
                    
                    patrolArea = {
                        spawn.x, 
                        spawn.y - mapManager.GetScrollY(),
                        spawn.width,
                        spawn.height
                    };
                }
                
                std::vector<Vector2> screenPath;
                for (const auto& wp : spawn.path) {
                    screenPath.push_back({ wp.x, wp.y - mapManager.GetScrollY() });
                }

                // Fábrica de Inimigos: hoje todo enemyType cai no Tank.
                // Quando existir um segundo inimigo, é aqui que o tipo escolhe a classe.
                auto t = std::make_unique<Tank>();
                t->Initialize({ screenX, screenY }, spawn.direction, 0, screenPath, patrolArea);
                enemies.push_back(std::move(t));
            }
        }
    }
    
    // Sistema Anti-Trombada (Separation Behavior)
    for (size_t i = 0; i < enemies.size(); i++) {
        for (size_t j = i + 1; j < enemies.size(); j++) {
            if (enemies[i]->isDestroyed || enemies[j]->isDestroyed) continue;
            
            Rectangle r1 = enemies[i]->GetHitbox();
            Rectangle r2 = enemies[j]->GetHitbox();
            if (CheckCollisionRecs(r1, r2)) {
                Vector2 c1 = { r1.x + r1.width/2.0f, r1.y + r1.height/2.0f };
                Vector2 c2 = { r2.x + r2.width/2.0f, r2.y + r2.height/2.0f };
                float dx = c1.x - c2.x;
                float dy = c1.y - c2.y;
                float dist = sqrt(dx*dx + dy*dy);
                if (dist == 0.0f) { dx = 1.0f; dist = 1.0f; }
                
                // Força de repulsão suave (Bumper car)
                float pushForce = 30.0f * deltaTime;
                enemies[i]->position.x += (dx/dist) * pushForce;
                enemies[i]->position.y += (dy/dist) * pushForce;
                enemies[j]->position.x -= (dx/dist) * pushForce;
                enemies[j]->position.y -= (dy/dist) * pushForce;
            }
        }
    }

    // Atualiza os tanques, checa colisão com tiro, e remove os inativos
    for (auto& e : enemies)
    {
        // Aplica o movimento da câmera sobre todos os inimigos (Ilusão de movimento)
        e->position.y += scrollSpeed * deltaTime;

        e->Update(deltaTime, player.GetPosition(), player.isDestroyed, scrollSpeed);

        // Se o tanque atirou neste frame, instanciamos a bala inimiga!
        if (e->hasFired)
        {
            e->hasFired = false;

            // O Tank usa rotação 0 = para BAIXO (Y+)
            float rad = e->cannonRotation * DEG2RAD;
            Vector2 forward = { -sinf(rad), cosf(rad) };

            // Spawna na ponta do cano
            float offset = std::abs(e->fireOffsetY);
            Vector2 startPos = { e->position.x + (forward.x * offset), e->position.y + (forward.y * offset) };
            EnemyBullet bullet;
            bullet.Initialize(startPos, forward, 250.0f);
            enemyBullets.push_back(bullet);
        }

        if (e->isDestroyed)
        {
            e->hitTimer -= deltaTime;
        }

        // Só tenta matar o tanque se ele já não estiver destruído
        if (!e->isDestroyed)
        {
            if (player.CheckBulletHits(e->GetHitbox()))
            {
                e->TakeDamage(1); // Arranca 1 de HP por bala

                // Exibe a faísca (FireElement1) em um ponto aleatório dentro da hitbox
                Rectangle hit = e->GetHitbox();
                float randX = hit.x + (float)GetRandomValue(0, (int)hit.width);
                float randY = hit.y + (float)GetRandomValue(0, (int)hit.height);
                explosionManager.Spawn({randX, randY}, ExplosionType::TYPE_2, 0.5f);

                // Se esse tiro acabou de destruir o tanque
                if (e->hp <= 0 && e->isDestroyed)
                {
                    explosionManager.Spawn(e->position, ExplosionType::TYPE_0, 1.0f);
                }
                else
                {
                    // Escolhe 1 dos 5 sons de impacto aleatoriamente (só se não explodiu)
                    int randSfx = GetRandomValue(0, IMPACT_SOUND_COUNT - 1);
                    if (impactSounds[randSfx].frameCount != 0)
                    {
                        SetSoundVolume(impactSounds[randSfx], 0.7f);
                        PlaySound(impactSounds[randSfx]);
                    }
                }
            }
        }
    }

    // Remove os inimigos que sairam da tela
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
        [](const std::unique_ptr<EnemyBase>& e) { return !e->isActive; }), enemies.end());
    
    // Anima e remove explosões
    explosionManager.Update(deltaTime, scrollSpeed);
    
    // Atualiza Balas Inimigas
    for (auto& b : enemyBullets)
    {
        // Movimento e rastro ficam dentro do próprio EnemyBullet
        b.Update(deltaTime);

        // Colisão com o Jogador
        if (b.active && !player.isDestroyed && CheckCollisionPointRec(b.position, player.GetHitbox()))
        {
            player.TakeDamage(20); // Bala de tanque arranca 20 de vida
            b.OnHit();

            // Explode na carcaça do player
            explosionManager.Spawn(b.position, ExplosionType::TYPE_0, 0.5f);
        }
    }

    // Descarta as balas que sairam da tela ou que já dissiparam toda a fumaça
    enemyBullets.erase(std::remove_if(enemyBullets.begin(), enemyBullets.end(),
        [](const EnemyBullet& b) {
            const bool foraDaTela = b.position.x < -Config::CULL_MARGIN ||
                                    b.position.x > Config::SCREEN_WIDTH + Config::CULL_MARGIN ||
                                    b.position.y < -Config::CULL_MARGIN ||
                                    b.position.y > Config::SCREEN_HEIGHT + Config::CULL_MARGIN;
            return foraDaTela || (!b.active && b.trail.empty());
        }), enemyBullets.end());
}

void Game::Render()
{
    // ETAPA 1: DESENHO PRINCIPAL NA TELA
    BeginDrawing();
    ClearBackground({ 34, 139, 34, 255 }); // Verde Floresta Escuro temporario

    // Desenha o fundo do mapa (Chão, Água)
    mapManager.Render();

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


void Game::Shutdown()
{
    // Tudo aqui precisa rodar ANTES de CloseWindow()/CloseAudioDevice(),
    // senão liberamos texturas com o contexto gráfico já destruído.
    enemies.clear();
    enemyBullets.clear();

    if (bgMusic.frameCount != 0) UnloadMusicStream(bgMusic);

    for (int i = 0; i < IMPACT_SOUND_COUNT; i++)
    {
        if (impactSounds[i].frameCount != 0) UnloadSound(impactSounds[i]);
    }

    UnloadRenderTexture(globalShadowTarget);

    player.Unload();
    Tank::UnloadSharedAssets();
    explosionManager.Unload();
    smokeManager.Unload();
    mapManager.Unload();
}

