#include "Game.h"
#include "Constants.h"
#include <raylib.h>
#include <algorithm>
#include <cmath>


void Game::Initialize()
{
    enemies.clear();
    explosionManager.Clear();
    enemyBullets.clear();
    playerMissiles.clear();
    playerBombs.clear();
    score = 0;

    mapManager.Load("assets/maps/level1.json");

    // O helicóptero nasce onde o objeto PlayerStartPosition estiver no mapa. Como o
    // Tiled dá a posição em coordenadas de mundo, tiramos o scroll inicial para
    // chegar na tela. Sem o objeto no mapa, cai no centro embaixo.
    Vector2 playerStart = { Config::SCREEN_WIDTH / 2.0f, 600.0f };
    if (mapManager.HasPlayerStart())
    {
        const Vector2 world = mapManager.GetPlayerStart();
        playerStart = { world.x, world.y - mapManager.GetScrollY() };
    }

    player.Initialize(playerStart);

    scrollSpeed = 75.0f; // Pixels por segundo
    
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
    
    // Carrega os Sons de Impacto Metálico. 3 vozes cada: a metralhadora
    // acerta rápido demais para um som só dar conta.
    for (int i = 0; i < IMPACT_SOUND_COUNT; i++)
    {
        impactSounds[i].Load(TextFormat("assets/audio/metal_impact/impact%d.ogg", i + 1), 3);
    }

    bombExplosionSound = LoadSound("assets/audio/helicopter/bomb-explosion.ogg");
    if (bombExplosionSound.frameCount != 0) SetSoundVolume(bombExplosionSound, 0.7f);

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

    // FIM DE FASE: o mapa acabou de rolar, então o helicóptero vai pousar
    if (mapManager.IsAtEnd() && mapManager.HasPlayerFinish() &&
        !player.IsLanding() && !player.isDestroyed)
    {
        const Vector2 world = mapManager.GetPlayerFinish();
        player.StartLanding({ world.x, world.y - mapManager.GetScrollY() });
    }

    player.Update(deltaTime);
    
    // Se o player acabou de bater no chão após ser destruído, spawnar a explosão!
    if (player.justHitGround)
    {
        player.justHitGround = false;
        // Explosão 3 é a do player, um pouco maior
        explosionManager.Spawn(player.GetPosition(), ExplosionType::TYPE_1, 1.5f);
        
        // Se tivermos um som de explosão de player, tocaríamos aqui
    }

    // Míssil: o Player só levanta a flag; quem cria e gerencia o míssil de
    // verdade é o Game, mesmo padrão da bala do tanque (EnemyBullet).
    if (player.hasFiredMissile)
    {
        player.hasFiredMissile = false;
        PlayerMissile m;
        m.Initialize(player.GetMissileSpawnPos());
        playerMissiles.push_back(m);
    }

    for (auto& m : playerMissiles)
    {
        m.Update(deltaTime);
    }

    // Bomba: mesmo padrão do míssil, mas herda a velocidade atual do
    // helicóptero no instante da soltura (inércia) em vez de sair propulsada.
    if (player.hasDroppedBomb)
    {
        player.hasDroppedBomb = false;
        PlayerBomb b;
        b.Initialize(player.GetBombDropPosition(), player.GetVelocity());
        playerBombs.push_back(b);
    }

    for (auto& b : playerBombs)
    {
        b.Update(deltaTime);
    }

    // Bombas que já terminaram de cair: explodem (efeito visual reaproveitado
    // do ExplosionManager, mais o som próprio da bomba), causam dano em área
    // e saem da lista.
    for (const auto& b : playerBombs)
    {
        if (b.HasLanded())
        {
            const Vector2 blastCenter = b.GetPosition();

            // Escala bem menor que a do helicóptero batendo no chão (1.5) —
            // a bomba é pequena e cai perto do próprio helicóptero, uma
            // explosão grande demais fica desproporcional.
            const float bombExplosionScale = 0.6f;
            explosionManager.Spawn(blastCenter, ExplosionType::TYPE_1, bombExplosionScale);
            if (bombExplosionSound.frameCount != 0) PlaySound(bombExplosionSound);

            // Raio do estilhaço: o dobro do raio VISUAL da explosão (o
            // sprite de Explosion1 é 256px; na escala da bomba, isso dá um
            // raio desenhado de 76.8px — o estilhaço abre o dobro disso).
            const float explosionVisualRadius = (256.0f * bombExplosionScale) / 2.0f;
            const float blastRadius = explosionVisualRadius * 2.0f;

            // Dano no centro do estilhaço — cai linearmente até 0 na borda
            // do raio (quanto mais no meio, mais dano; quanto mais na
            // borda, menos dano).
            const int maxBlastDamage = 9999;
            auto DanoPelaDistancia = [&](Vector2 alvoCentro) -> int
            {
                const float dx = alvoCentro.x - blastCenter.x;
                const float dy = alvoCentro.y - blastCenter.y;
                const float dist = sqrtf(dx * dx + dy * dy);
                if (dist >= blastRadius) return 0;

                const float falloff = 1.0f - (dist / blastRadius);
                return (int)(maxBlastDamage * falloff);
            };

            // Tanques dentro do raio
            for (auto& e : enemies)
            {
                if (e->isDestroyed) continue;

                Rectangle hit = e->GetHitbox();
                Vector2 centro = { hit.x + hit.width / 2.0f, hit.y + hit.height / 2.0f };
                int dano = DanoPelaDistancia(centro);
                if (dano <= 0) continue;

                e->TakeDamage(dano);
                if (e->hp <= 0 && e->isDestroyed)
                {
                    score += 100;
                    explosionManager.Spawn(e->position, ExplosionType::TYPE_3, 1.0f);
                }
            }

            // Construções dentro do raio
            for (int i = 0; i < mapManager.GetObjectCount(); i++)
            {
                if (!mapManager.IsDestructible(i) || mapManager.IsObjectDestroyed(i)) continue;

                Rectangle box = mapManager.GetObjectHitbox(i);
                Vector2 centro = { box.x + box.width / 2.0f, box.y + box.height / 2.0f };
                int dano = DanoPelaDistancia(centro);
                if (dano <= 0) continue;

                if (mapManager.DamageObject(i, dano))
                {
                    score += 50;
                    explosionManager.Spawn(centro, ExplosionType::TYPE_0, 2.0f);
                }
            }
        }
    }
    playerBombs.erase(std::remove_if(playerBombs.begin(), playerBombs.end(),
        [](const PlayerBomb& b) { return b.HasLanded(); }), playerBombs.end());

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

        // Só tenta matar o tanque se ele já não estiver destruído
        if (!e->isDestroyed)
        {
            // Aplica o dano de um tiro que acertou o tanque; primária e
            // secundária diferem só no valor de dano (a secundária é mais forte).
            auto aplicarTiroNoTanque = [&](int dano)
            {
                e->TakeDamage(dano);

                // Exibe a faísca (FireElement1) em um ponto aleatório dentro da hitbox
                Rectangle hit = e->GetHitbox();
                float randX = hit.x + (float)GetRandomValue(0, (int)hit.width);
                float randY = hit.y + (float)GetRandomValue(0, (int)hit.height);
                explosionManager.Spawn({randX, randY}, ExplosionType::TYPE_2, 0.5f);

                // Se esse tiro acabou de destruir o tanque
                if (e->hp <= 0 && e->isDestroyed)
                {
                    score += 100; // Tanque destruído
                    explosionManager.Spawn(e->position, ExplosionType::TYPE_3, 1.0f);
                }
                else
                {
                    // Só faz barulho de metal se o tanque aguentou o tiro
                    PlayImpactSound();
                }
            };

            if (player.CheckBulletHits(e->GetHitbox())) aplicarTiroNoTanque(1);

            // Se o próprio tiro primário já destruiu o tanque, não checa a
            // secundária no mesmo frame (ele já não existe mais como alvo)
            if (!e->isDestroyed && player.CheckSecondaryBulletHits(e->GetHitbox()))
            {
                aplicarTiroNoTanque(5); // Tiro secundário: 5x mais forte
            }

            // Míssil: destrói em 1 tiro (é anti-blindado de verdade). Um
            // míssil só atinge um alvo, por isso o break.
            if (!e->isDestroyed)
            {
                for (auto& m : playerMissiles)
                {
                    if (!m.active) continue;
                    if (CheckCollisionRecs(m.GetHitbox(), e->GetHitbox()))
                    {
                        m.OnHit();
                        aplicarTiroNoTanque(9999);
                        break;
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

        // Aplica o dano de um tiro que acertou a construção; primária e
        // secundária diferem só no valor de dano.
        auto aplicarTiroNaConstrucao = [&](int dano)
        {
            // Faísca no ponto do impacto
            float randX = box.x + (float)GetRandomValue(0, (int)box.width);
            float randY = box.y + (float)GetRandomValue(0, (int)box.height);
            explosionManager.Spawn({ randX, randY }, ExplosionType::TYPE_2, 0.5f);

            Vector2 centro = { box.x + box.width / 2.0f, box.y + box.height / 2.0f };

            if (mapManager.DamageObject(i, dano))
            {
                // Só o tiro que derrubou entra aqui
                score += 50; // Construção derrubada
                explosionManager.Spawn(centro, ExplosionType::TYPE_0, 2.0f);
            }
            else
            {
                PlayImpactSound();
            }
        };

        if (player.CheckBulletHits(box)) aplicarTiroNaConstrucao(1);

        if (!mapManager.IsObjectDestroyed(i) && player.CheckSecondaryBulletHits(box))
        {
            aplicarTiroNaConstrucao(5); // Tiro secundário: 5x mais forte
        }

        // Míssil: derruba em 1 tiro
        if (!mapManager.IsObjectDestroyed(i))
        {
            for (auto& m : playerMissiles)
            {
                if (!m.active) continue;
                if (CheckCollisionRecs(m.GetHitbox(), box))
                {
                    m.OnHit();
                    aplicarTiroNaConstrucao(9999);
                    break;
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
        // Círculo, e não ponto: a bala é desenhada com 7 px de raio, então o
        // ponto matemático deixava passar tiros que visivelmente encostaram.
        if (b.active && !player.isDestroyed &&
            CheckCollisionCircleRec(b.position, EnemyBullet::RADIUS, player.GetHitbox()))
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

    // Descarta os mísseis que sairam da tela ou que já dissiparam o rastro
    playerMissiles.erase(std::remove_if(playerMissiles.begin(), playerMissiles.end(),
        [](const PlayerMissile& m) {
            const bool foraDaTela = m.position.y < -Config::CULL_MARGIN;
            return foraDaTela || (!m.active && m.trail.empty());
        }), playerMissiles.end());
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

    // ETAPA 3: CARIMBA AS SOMBRAS DO CHÃO
    StampShadows();

    // ETAPA 3.5: AS CONSTRUÇÕES, POR CIMA DA PRÓPRIA SOMBRA
    mapManager.RenderObjects();

    // ETAPA 4: DESENHA AS CORES REAIS DOS OBJETOS POR CIMA DA SOMBRA
    for (const auto& e : enemies) e->DrawBody();

    // ETAPA 4.5: SOMBRA DO HELICÓPTERO (E DOS MÍSSEIS/BOMBAS), POR CIMA DE
    // TUDO QUE ESTÁ NO CHÃO. Ele voa acima do cenário, então a sombra dele
    // tem que cair SOBRE as casas e os tanques. Reaproveitamos o mesmo
    // canvas: limpa e usa de novo.
    BeginTextureMode(globalShadowTarget);
        ClearBackground(BLANK);
        player.DrawShadows();
        for (const auto& m : playerMissiles) m.DrawShadows();
        for (const auto& b : playerBombs) b.DrawShadows();
    EndTextureMode();

    StampShadows();

    // Mísseis e bombas do player: desenhados ANTES do corpo do helicóptero,
    // pra ficarem por baixo dele (igual aos mísseis decorativos das asas).
    for (const auto& m : playerMissiles)
    {
        m.Render();
    }
    for (const auto& b : playerBombs)
    {
        b.Render();
    }

    // Explosões: efeito de CHÃO (tanque, construção, bomba) — precisa ficar
    // por baixo do helicóptero, que voa acima de tudo. Antes ficava depois
    // de player.DrawBody() e a explosão da bomba (que cai bem perto do
    // helicóptero) acabava cobrindo ele por cima.
    explosionManager.Render();

    // ETAPA 4.6: E só então o helicóptero em si
    player.DrawBody();

    // ETAPA 5: DESENHAR FUMAÇAS POR CIMA DE TUDO
    for (const auto& e : enemies)
    {
        if (e->isDestroyed) smokeManager.Render(e->position, e->smokeFrame);
    }

    // Tiros Inimigos
    for (const auto& b : enemyBullets)
    {
        b.Render();
    }

    // UI
    RenderHUD();
    DrawFPS(10, 10);

    EndDrawing();


}


void Game::PlayImpactSound()
{
    const int randSfx = GetRandomValue(0, IMPACT_SOUND_COUNT - 1);
    impactSounds[randSfx].Play(0.7f);
}

// Cores do HUD: quanto menos vida sobra, mais a barra pende pro vermelho.
static Color HpBarColor(float ratio)
{
    if (ratio > 0.5f) return GREEN;
    if (ratio > 0.25f) return YELLOW;
    return RED;
}

// =====================================================================
// HUD DE MUNIÇÃO. Míssil e bomba já leem munição de verdade do Player; o
// slot da secundária ainda é número fixo — ela já atira, mas ainda não
// tem munição limitada.
// =====================================================================
enum class ArmaTipo { Secundaria, Missil, Bomba };
struct MunicaoSlot { ArmaTipo tipo; Color cor; int atual; int maximo; };

static void DrawMunicaoSlot(float x, float y, const MunicaoSlot& slot)
{
    // Um ícone por arma. Sem a imagem, cai no quadradinho colorido (mesmo
    // carregamento defensivo do resto do jogo).
    static Texture2D iconeSecundaria = LoadTexture("assets/hud/secondary_arm.png");
    static Texture2D iconeMissil = LoadTexture("assets/hud/missil.png");
    static Texture2D iconeBomba = LoadTexture("assets/hud/bomb.png");

    Texture2D iconeArma = {};
    switch (slot.tipo)
    {
        case ArmaTipo::Secundaria: iconeArma = iconeSecundaria; break;
        case ArmaTipo::Missil:     iconeArma = iconeMissil;     break;
        case ArmaTipo::Bomba:      iconeArma = iconeBomba;      break;
    }

    const float escala = 0.7f;
    const float boxSize = (iconeArma.id != 0) ? (iconeArma.width * escala) : 28.0f;

    if (iconeArma.id != 0)
    {
        // Sem tingir: mostra a arte do ícone com as cores originais
        DrawTextureEx(iconeArma, { x, y }, 0.0f, escala, WHITE);
    }
    else
    {
        // Sem a imagem, a cor é o que diferencia o quadradinho de
        // placeholder de um slot do outro
        DrawRectangle((int)x, (int)y, (int)boxSize, (int)boxSize, slot.cor);
        DrawRectangleLines((int)x, (int)y, (int)boxSize, (int)boxSize, WHITE);
    }

    // Número, no formato atual/máximo, embaixo do ícone e centralizado nele
    const char* texto = TextFormat("%d/%d", slot.atual, slot.maximo);
    const int fontSize = 18;
    const int textoWidth = MeasureText(texto, fontSize);
    DrawText(texto, (int)(x + boxSize / 2.0f - textoWidth / 2.0f), (int)(y + boxSize), fontSize, WHITE);
}

static void RenderMunicaoMockup(const Player& player)
{
    // Canto inferior esquerdo: não disputa espaço com a área de leitura
    // vertical (o que importa num scroller vertical é ver o que vem por cima).
    // baseY reserva espaço pro ícone + o número que agora fica embaixo dele —
    // se aumentar bastante a escala do ícone, precisa afastar isso do rodapé.
    const float baseX = 10.0f;
    const float baseY = (float)Config::SCREEN_HEIGHT - 100.0f;
    const float espacamento = 100.0f; // dá respiro entre os 3 ícones

    MunicaoSlot slots[3] = {
        { ArmaTipo::Secundaria, YELLOW, 12, 20 },                                    // ainda mockado
        { ArmaTipo::Missil,     ORANGE, player.missileAmmo, player.missileMaxAmmo }, // real
        { ArmaTipo::Bomba,      RED,    player.bombAmmo,    player.bombMaxAmmo   }, // real
    };

    for (int i = 0; i < 3; i++)
    {
        DrawMunicaoSlot(baseX + i * espacamento, baseY, slots[i]);
    }
}

void Game::RenderHUD() const
{
    RenderMunicaoMockup(player);

    // --- Barra de vida, canto superior esquerdo (abaixo do contador de FPS,
    // que o raylib desenha nos primeiros ~30px com DrawFPS) ---
    const float hpBarX = 10.0f, hpBarY = 55.0f, hpBarW = 200.0f, hpBarH = 20.0f;
    const float hpRatio = player.GetHpRatio();

    DrawRectangle((int)hpBarX, (int)hpBarY, (int)hpBarW, (int)hpBarH, DARKGRAY);
    DrawRectangle((int)hpBarX, (int)hpBarY, (int)(hpBarW * hpRatio), (int)hpBarH, HpBarColor(hpRatio));
    DrawRectangleLines((int)hpBarX, (int)hpBarY, (int)hpBarW, (int)hpBarH, WHITE);
    DrawText("VIDA", (int)hpBarX, (int)(hpBarY - 18.0f), 16, WHITE);

    // --- Pontuação, canto superior direito ---
    const char* scoreText = TextFormat("PONTOS: %d", score);
    const int scoreFontSize = 20;
    const int scoreWidth = MeasureText(scoreText, scoreFontSize);
    DrawText(scoreText, Config::SCREEN_WIDTH - scoreWidth - 10, 10, scoreFontSize, WHITE);

    // --- Progresso da fase, barra fina no topo da tela ---
    const float progress = mapManager.GetProgress();
    const float progressBarY = 0.0f, progressBarH = 6.0f;
    DrawRectangle(0, (int)progressBarY, Config::SCREEN_WIDTH, (int)progressBarH, DARKGRAY);
    DrawRectangle(0, (int)progressBarY, (int)(Config::SCREEN_WIDTH * progress), (int)progressBarH, SKYBLUE);
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
    playerMissiles.clear();
    playerBombs.clear();

    if (bgMusic.frameCount != 0) UnloadMusicStream(bgMusic);

    for (int i = 0; i < IMPACT_SOUND_COUNT; i++) impactSounds[i].Unload();
    if (bombExplosionSound.frameCount != 0) { UnloadSound(bombExplosionSound); bombExplosionSound = {}; }

    UnloadRenderTexture(globalShadowTarget);

    player.Unload();
    Tank::UnloadSharedAssets();
    PlayerMissile::UnloadSharedAssets();
    PlayerBomb::UnloadSharedAssets();
    explosionManager.Unload();
    smokeManager.Unload();
    mapManager.Unload();
}

