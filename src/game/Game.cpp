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

    enemies.clear();
    explosionManager.Clear();
    enemyBullets.clear();
    tankSpawnTimer = 0.0f;

    mapManager.Load("assets/maps/level1.json");

    // O helicóptero nasce onde o objeto PlayerPosition estiver no mapa. Como o
    // Tiled dá a posição em coordenadas de mundo, tiramos o scroll inicial para
    // chegar na tela. Sem o objeto no mapa, cai no centro embaixo.
    Vector2 playerStart = { Config::SCREEN_WIDTH / 2.0f, 600.0f };
    if (mapManager.HasPlayerStart())
    {
        const Vector2 world = mapManager.GetPlayerStart();
        playerStart = { world.x, world.y - mapManager.GetScrollY() };
    }

    player.Initialize(playerStart);

    scrollSpeed = 35.0f; // Pixels por segundo
    
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
    // O cenário só começa a rolar depois que o helicóptero decola. Antes disso
    // ele ainda está subindo do chão, e o mapa fica parado esperando.
    const float targetScroll = player.IsAirborne() ? scrollSpeed : 0.0f;

    // Velocidade com que o cenário andou DE FATO neste frame. No fim do mapa
    // o scroll trava, e os inimigos precisam travar junto: se continuassem
    // descendo, sairiam do traçado desenhado no Tiled.
    const float scrolled = mapManager.Update(deltaTime, targetScroll);
    const float worldScroll = (deltaTime > 0.0f) ? (scrolled / deltaTime) : 0.0f;

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

                // Fábrica de Inimigos. Hoje os dois tipos são Tank, mudando só
                // o modelo; quando existir outra classe de inimigo é aqui que ela
                // entra, olhando o mesmo spawn.enemyType.
                const int tankType = (spawn.enemyType == "HEAVY_TANK") ? 1 : 0;

                auto t = std::make_unique<Tank>();
                t->Initialize({ screenX, screenY }, spawn.direction, tankType, screenPath, patrolArea, spawn.stats);
                enemies.push_back(std::move(t));
            }
        }
    }
    
    // Sistema Anti-Trombada (Ceder Passagem)
    // Empurrar a posição tirava o tanque do traçado desenhado no Tiled, e ele
    // acabava atravessando lugares por onde a rota não passa. Agora ninguém sai
    // do lugar: quem está indo em cima do outro só freia e espera a passagem.
    for (auto& e : enemies) e->mustYield = false;

    for (size_t i = 0; i < enemies.size(); i++) {
        for (size_t j = i + 1; j < enemies.size(); j++) {
            if (enemies[i]->isDestroyed || enemies[j]->isDestroyed) continue;

            Rectangle r1 = enemies[i]->GetHitbox();
            Rectangle r2 = enemies[j]->GetHitbox();
            if (!CheckCollisionRecs(r1, r2)) continue;

            Vector2 c1 = { r1.x + r1.width/2.0f, r1.y + r1.height/2.0f };
            Vector2 c2 = { r2.x + r2.width/2.0f, r2.y + r2.height/2.0f };

            // Vetor unitário que aponta de i para j
            float dx = c2.x - c1.x;
            float dy = c2.y - c1.y;
            float dist = sqrtf(dx*dx + dy*dy);
            if (dist < 0.001f) { dx = 0.0f; dy = 1.0f; dist = 1.0f; }

            float nx = dx / dist;
            float ny = dy / dist;

            // O quanto cada um está avançando na direção do outro
            float avancoI =  (enemies[i]->velocity.x * nx) + (enemies[i]->velocity.y * ny);
            float avancoJ = -((enemies[j]->velocity.x * nx) + (enemies[j]->velocity.y * ny));

            // Encostados mas já se separando: não há trombada a evitar
            if (avancoI <= 0.0f && avancoJ <= 0.0f) continue;

            // Cede só quem avança mais. Se os dois cedessem numa batida de
            // frente, ambos parariam colados e nunca mais sairiam dali.
            if (avancoI >= avancoJ) enemies[i]->mustYield = true;
            else                    enemies[j]->mustYield = true;
        }
    }

    // Atualiza os tanques, checa colisão com tiro, e remove os inativos
    for (auto& e : enemies)
    {
        // Aplica o movimento da câmera sobre todos os inimigos (Ilusão de movimento)
        e->position.y += worldScroll * deltaTime;

        e->Update(deltaTime, player.GetPosition(), player.isDestroyed, worldScroll);

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
                    explosionManager.Spawn(e->position, ExplosionType::TYPE_3, 1.0f);
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

    // Tiros do player contra as construções (classe Building do Tiled)
    for (int i = 0; i < mapManager.GetObjectCount(); i++)
    {
        // Sem HP no mapa é cenário puro: o tiro passa direto
        if (!mapManager.IsDestructible(i) || mapManager.IsObjectDestroyed(i)) continue;

        Rectangle box = mapManager.GetObjectHitbox(i);

        // Fora da tela não precisa ser testada
        if (box.y + box.height < 0.0f || box.y > Config::SCREEN_HEIGHT) continue;

        if (player.CheckBulletHits(box))
        {
            // Faísca no ponto do impacto
            float randX = box.x + (float)GetRandomValue(0, (int)box.width);
            float randY = box.y + (float)GetRandomValue(0, (int)box.height);
            explosionManager.Spawn({ randX, randY }, ExplosionType::TYPE_2, 0.5f);

            Vector2 centro = { box.x + box.width / 2.0f, box.y + box.height / 2.0f };

            if (mapManager.DamageObject(i, 1))
            {
                // Só o tiro que derrubou entra aqui
                explosionManager.Spawn(centro, ExplosionType::TYPE_0, 2.0f);
            }
            else
            {
                int randSfx = GetRandomValue(0, IMPACT_SOUND_COUNT - 1);
                if (impactSounds[randSfx].frameCount != 0)
                {
                    SetSoundVolume(impactSounds[randSfx], 0.7f);
                    PlaySound(impactSounds[randSfx]);
                }
            }
        }
    }
    
    // Anima e remove explosões
    explosionManager.Update(deltaTime, worldScroll);
    
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

    // Desenha o fundo do mapa (Chão, Água). As construções NÃO entram aqui:
    // a sombra delas precisa ser carimbada antes delas próprias.
    mapManager.RenderGround();

    // ETAPA 2: SHADOW PASS DO QUE ESTÁ NO CHÃO (construções e inimigos).
    // O player fica de fora de propósito: ele voa, e a sombra dele é carimbada
    // só lá embaixo, depois que casas e tanques já estiverem desenhados.
    BeginTextureMode(globalShadowTarget);
        ClearBackground(BLANK); // Limpa o fundo do buffer com alfa 0

        // Sombra das construções
        mapManager.RenderObjectShadows();

        // Desenha a sombra de todos os inimigos
        for (const auto& e : enemies) e->DrawShadows();
    EndTextureMode();

    BeginMode2D(camera);
        // Quando criarmos as tilesets (chao e agua), desenhamos elas aqui embaixo!
        for (const auto& e : enemies) e->DrawGroundEffects();
    EndMode2D();

    // ETAPA 3: CARIMBA AS SOMBRAS DO CHÃO
    StampShadows();

    // ETAPA 3.5: AS CONSTRUÇÕES, POR CIMA DA PRÓPRIA SOMBRA
    mapManager.RenderObjects();

    // ETAPA 4: DESENHA AS CORES REAIS DOS OBJETOS POR CIMA DA SOMBRA
    for (const auto& e : enemies) e->DrawBody();

    // ETAPA 4.5: SOMBRA DO HELICÓPTERO, POR CIMA DE TUDO QUE ESTÁ NO CHÃO.
    // Ele voa acima do cenário, então a sombra dele tem que cair SOBRE as
    // casas e os tanques. Reaproveitamos o mesmo canvas: limpa e usa de novo.
    BeginTextureMode(globalShadowTarget);
        ClearBackground(BLANK);
        player.DrawShadows();
    EndTextureMode();

    StampShadows();

    // ETAPA 4.6: E só então o helicóptero em si
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


void Game::StampShadows()
{
    // O height negativo no source inverte a textura: render target do OpenGL
    // vem de cabeça para baixo.
    Rectangle sourceRec = { 0.0f, 0.0f, (float)globalShadowTarget.texture.width, -(float)globalShadowTarget.texture.height };
    Rectangle destRec = { 0.0f, 0.0f, (float)globalShadowTarget.texture.width, (float)globalShadowTarget.texture.height };

    // Aplica o nivel de transparencia nas sombras unificadas!
    DrawTexturePro(globalShadowTarget.texture, sourceRec, destRec, { 0.0f, 0.0f }, 0.0f, { 255, 255, 255, 80 });
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

