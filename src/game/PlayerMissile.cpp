#include "PlayerMissile.h"

// Física do motor-foguete: sai devagar e acelera até a velocidade de cruzeiro
// nos primeiros instantes (fase de impulso), depois mantém constante (fase de
// sustentação) — é assim que o motor de um míssil real como o Hellfire
// funciona, diferente das balas do jogo, que já nascem na velocidade final.
static const float BOOST_DURATION = 0.35f;  // segundos até atingir a velocidade máxima
static const float INITIAL_SPEED = 1.0f;  // velocidade ao sair do trilho
static const float CRUISE_SPEED = 700.0f;  // velocidade de cruzeiro (o projétil mais rápido do jogo)

// O sprite bruto (26x100) tem quase metade da altura do helicóptero
// (90x206) — grande demais pra um armamento subalar. Essa escala deixa a
// altura do míssil em ~32% da altura do helicóptero.
static const float MISSILE_SCALE = 0.40f;

// Distância da sombra: o míssil só voa com o helicóptero em altitude
// máxima (scale=1.0), então usa a MESMA distância que o Player calcula
// pra esse instante (Player::DrawShadows: 15 + (scale-0.6)*100 = 55 em
// scale=1.0). Fixa porque o míssil não sobe nem desce.
static const float MISSILE_SHADOW_OFFSET = 55.0f;

static Texture2D missileSprite = {};
static bool missileTextureLoaded = false;

void PlayerMissile::Initialize(Vector2 startPos)
{
    position = startPos;
    active = true;
    currentSpeed = INITIAL_SPEED;
    boostTimer = 0.0f;
    travel = 0.0f;
    trail.clear();
    trailTimer = 0.0f;

    if (!missileTextureLoaded)
    {
        missileSprite = LoadTexture("assets/sprites/helicopter/missil.png");
        missileTextureLoaded = true;
    }
}

void PlayerMissile::Update(float deltaTime)
{
    if (active)
    {
        // Fase de impulso: acelera até a velocidade de cruzeiro; depois,
        // sustentação constante.
        if (boostTimer < BOOST_DURATION)
        {
            boostTimer += deltaTime;
            float t = (boostTimer < BOOST_DURATION) ? (boostTimer / BOOST_DURATION) : 1.0f;
            currentSpeed = INITIAL_SPEED + (CRUISE_SPEED - INITIAL_SPEED) * t;
        }
        else
        {
            currentSpeed = CRUISE_SPEED;
        }

        travel = currentSpeed * deltaTime;
        position.y -= travel;

        // Rastro de fumaça do motor-foguete (mais grosso e mais claro que o
        // da bala inimiga, que é fumaça de pólvora dissipando). Nasce na
        // CAUDA do míssil, não no centro do sprite: como o nariz aponta pra
        // cima, a cauda fica meia altura ABAIXO da posição (y maior).
        const float halfHeight = (missileSprite.id != 0) ? (missileSprite.height * MISSILE_SCALE) / 2.0f : 0.0f;
        Vector2 tailPos = { position.x, position.y + halfHeight };

        trailTimer += deltaTime;
        if (trailTimer >= 0.02f)
        {
            trailTimer = 0.0f;
            trail.push_back(tailPos);
            if (trail.size() > 16) trail.erase(trail.begin());
        }
    }
    else
    {
        // Explodiu: só dissipa o rastro que sobrou
        trailTimer += deltaTime;
        if (trailTimer >= 0.02f && !trail.empty())
        {
            trailTimer = 0.0f;
            trail.erase(trail.begin());
        }
    }
}

void PlayerMissile::Render() const
{
    // Raio proporcional ao míssil (26px de largura * escala): antes o
    // rastro chegava a 20px de diâmetro, quase o dobro da largura do míssil.
    for (int i = 0; i < (int)trail.size(); i++)
    {
        float alpha = (float)i / (float)trail.size();
        float size = 1.5f + alpha * 2.5f;
        DrawCircleV(trail[i], size, Fade(LIGHTGRAY, alpha * 0.5f));
    }

    if (active && missileSprite.id != 0)
    {
        // O sprite já aponta nativamente pra cima, mesma direção do voo — sem rotação
        Vector2 drawPos = {
            position.x - (missileSprite.width * MISSILE_SCALE) / 2.0f,
            position.y - (missileSprite.height * MISSILE_SCALE) / 2.0f
        };
        DrawTextureEx(missileSprite, drawPos, 0.0f, MISSILE_SCALE, WHITE);
    }

    if (active)
    {
        // Chama de propulsão na cauda: núcleo branco-quente, envolto em
        // laranja e vermelho, por cima do sprite (fica na frente da cauda).
        // O jitter (variação aleatória de tamanho a cada frame) é o que dá
        // a sensação de chama tremulando, em vez de um círculo parado.
        const float halfHeight = (missileSprite.id != 0) ? (missileSprite.height * MISSILE_SCALE) / 2.0f : 0.0f;
        Vector2 tailPos = { position.x, position.y + halfHeight };
        const float jitter = (float)GetRandomValue(-10, 10) / 10.0f; // -1.0 a 1.0

        // Rabo comprido: linhas afunilando pra fora (mais grossa e mais
        // longa por fora, mais fina e curta por dentro), core redondo na ponta.
        DrawLineEx(tailPos, { tailPos.x, tailPos.y + 14.0f + jitter }, 5.0f + jitter, Fade(RED, 0.5f));
        DrawLineEx(tailPos, { tailPos.x, tailPos.y + 9.0f + jitter * 0.5f }, 3.5f + jitter * 0.5f, ORANGE);
        DrawCircleV(tailPos, 1.2f, YELLOW);
        DrawCircleV(tailPos, 0.6f, WHITE);
    }
}

void PlayerMissile::DrawShadows() const
{
    // Sombra do rastro de fumaça: mesmo perfil de tamanho/alpha do Render(),
    // só tingido de preto e deslocado. Continua mesmo depois do OnHit()
    // (igual o próprio rastro, que dissipa aos poucos).
    for (int i = 0; i < (int)trail.size(); i++)
    {
        float alpha = (float)i / (float)trail.size();
        float size = 1.5f + alpha * 2.5f;
        DrawCircleV({ trail[i].x + MISSILE_SHADOW_OFFSET, trail[i].y + MISSILE_SHADOW_OFFSET }, size, Fade(BLACK, alpha * 0.7f));
    }

    if (!active) return;

    // Sombra da chama de propulsão (mesmo formato do Render(), sem o
    // jitter — a sombra não precisa tremular igual à chama em si)
    const float halfHeight = (missileSprite.id != 0) ? (missileSprite.height * MISSILE_SCALE) / 2.0f : 0.0f;
    Vector2 tailShadow = { position.x + MISSILE_SHADOW_OFFSET, position.y + halfHeight + MISSILE_SHADOW_OFFSET };

    DrawLineEx(tailShadow, { tailShadow.x, tailShadow.y + 14.0f }, 5.0f, Fade(BLACK, 0.5f));
    DrawLineEx(tailShadow, { tailShadow.x, tailShadow.y + 9.0f }, 3.5f, Fade(BLACK, 0.5f));
    DrawCircleV(tailShadow, 1.2f, Fade(BLACK, 0.6f));

    if (missileSprite.id == 0) return;

    // Sombra do corpo do míssil
    Vector2 shadowDrawPos = {
        position.x - (missileSprite.width * MISSILE_SCALE) / 2.0f + MISSILE_SHADOW_OFFSET,
        position.y - (missileSprite.height * MISSILE_SCALE) / 2.0f + MISSILE_SHADOW_OFFSET
    };
    DrawTextureEx(missileSprite, shadowDrawPos, 0.0f, MISSILE_SCALE, BLACK);
}

Rectangle PlayerMissile::GetHitbox() const
{
    const float w = (missileSprite.id != 0) ? (float)missileSprite.width * MISSILE_SCALE : 10.0f;
    const float h = (missileSprite.id != 0) ? (float)missileSprite.height * MISSILE_SCALE : 40.0f;

    // Estende a caixa pelo trecho percorrido no frame — mesma lógica das
    // balas: na velocidade de cruzeiro (1600 px/s) ele anda ~27px por
    // quadro a 60 FPS, o suficiente pra pular por cima de um alvo fino sem
    // essa margem.
    return { position.x - w / 2.0f, position.y - h / 2.0f - travel, w, h + travel };
}

void PlayerMissile::OnHit()
{
    active = false;
}

void PlayerMissile::UnloadSharedAssets()
{
    if (missileTextureLoaded)
    {
        if (missileSprite.id != 0) UnloadTexture(missileSprite);
        missileSprite = {};
        missileTextureLoaded = false;
    }
}
