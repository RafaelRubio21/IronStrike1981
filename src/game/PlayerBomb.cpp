#include "PlayerBomb.h"

// Tempo de queda: do instante da soltura até tocar o chão.
static const float FALL_DURATION = 1.4f;

// Atrito do ar que vai freando o deslocamento horizontal herdado do
// helicóptero — sem isso ela derivaria pra sempre na mesma direção, reta,
// o que não parece uma bomba caindo.
static const float AIR_FRICTION = 1.5f;

// Mesma ideia de escala do míssil (ver PlayerMissile.cpp): o sprite bruto
// fica grande demais perto do helicóptero sem isso. É o tamanho no
// instante da soltura (ver CurrentScale — ela encolhe ao longo da queda).
static const float BOMB_SCALE = 0.5f;

// Conforme a bomba cai, ela encolhe na MESMA proporção que o helicóptero
// usa entre voar (scale 1.0) e ficar parado no chão (scale 0.5) — ou seja,
// termina a queda com metade do tamanho que tinha ao ser solta.
static const float BOMB_GROUND_SCALE_RATIO = 0.5f;

// Distância inicial da sombra: mesma altitude máxima que o helicóptero e
// o míssil usam (scale=1.0 -> 15 + 0.4*100 = 55). Encolhe até 0 conforme a
// bomba se aproxima do chão — ver FallProgress().
static const float BOMB_SHADOW_MAX_OFFSET = 55.0f;

static Texture2D bombSprite = {};
static bool bombTextureLoaded = false;

void PlayerBomb::Initialize(Vector2 startPos, Vector2 initialVelocity)
{
    position = startPos;
    velocity = initialVelocity;
    fallTimer = 0.0f;

    if (!bombTextureLoaded)
    {
        bombSprite = LoadTexture("assets/sprites/helicopter/bomb.png");
        bombTextureLoaded = true;
    }
}

void PlayerBomb::Update(float deltaTime)
{
    fallTimer += deltaTime;

    // Perde a inércia herdada do helicóptero por atrito com o ar — igual
    // ao próprio helicóptero (ver Player::Update), só que aqui nada
    // reacelera, então ela vai "endireitando" a queda aos poucos.
    velocity.x -= velocity.x * AIR_FRICTION * deltaTime;
    velocity.y -= velocity.y * AIR_FRICTION * deltaTime;

    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;
}

// Progresso da queda, de 0.0 (acabou de soltar) a 1.0 (tocando o chão)
static float FallProgress(float fallTimer)
{
    float t = fallTimer / FALL_DURATION;
    if (t > 1.0f) t = 1.0f;
    return t;
}

// Escala do sprite no instante atual: BOMB_SCALE cheia ao soltar, encolhendo
// linearmente até BOMB_SCALE*BOMB_GROUND_SCALE_RATIO no instante do impacto.
static float CurrentScale(float fallTimer)
{
    const float progress = FallProgress(fallTimer);
    return BOMB_SCALE * (1.0f - progress * (1.0f - BOMB_GROUND_SCALE_RATIO));
}

void PlayerBomb::Render() const
{
    if (bombSprite.id == 0) return;

    const float s = CurrentScale(fallTimer);

    // O sprite já aponta nativamente pra baixo (nariz embaixo, aletas em
    // cima) — mesma direção da queda, sem precisar de rotação.
    Vector2 drawPos = {
        position.x - (bombSprite.width * s) / 2.0f,
        position.y - (bombSprite.height * s) / 2.0f
    };
    DrawTextureEx(bombSprite, drawPos, 0.0f, s, WHITE);
}

void PlayerBomb::DrawShadows() const
{
    if (bombSprite.id == 0) return;

    const float s = CurrentScale(fallTimer);

    // A distância da sombra encolhe conforme a bomba se aproxima do chão,
    // até coincidir com o próprio sprite no instante do impacto.
    const float shadowOffset = BOMB_SHADOW_MAX_OFFSET * (1.0f - FallProgress(fallTimer));

    Vector2 shadowPos = {
        position.x - (bombSprite.width * s) / 2.0f + shadowOffset,
        position.y - (bombSprite.height * s) / 2.0f + shadowOffset
    };
    DrawTextureEx(bombSprite, shadowPos, 0.0f, s, BLACK);
}

bool PlayerBomb::HasLanded() const
{
    return fallTimer >= FALL_DURATION;
}

void PlayerBomb::UnloadSharedAssets()
{
    if (bombTextureLoaded)
    {
        if (bombSprite.id != 0) UnloadTexture(bombSprite);
        bombSprite = {};
        bombTextureLoaded = false;
    }
}
