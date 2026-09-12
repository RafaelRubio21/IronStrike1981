#pragma once
#include "raylib.h"
#include <vector>

// Míssil do player (tecla D). Física de lançamento inspirada num Hellfire de
// verdade: sai devagar do trilho e o motor-foguete acelera ele até a
// velocidade de cruzeiro nos primeiros instantes (fase de impulso), depois
// mantém constante (fase de sustentação). Sem guiamento: voa reto pra cima,
// igual às balas do resto do jogo.
//
// Quem é dono da lista de mísseis é o Game, não o Player — mesmo padrão do
// EnemyBullet do tanque. O Player só levanta a flag hasFiredMissile.
class PlayerMissile
{
public:
    void Initialize(Vector2 startPos);
    void Update(float deltaTime);
    void Render() const;
    void DrawShadows() const; // chamado no mesmo passe de sombra do helicóptero
    void OnHit();

    Rectangle GetHitbox() const;

    // Libera a textura compartilhada entre todos os mísseis. Precisa rodar
    // antes de CloseWindow().
    static void UnloadSharedAssets();

    Vector2 position;
    bool active;

    // Público, como no EnemyBullet: o Game decide quando descartar o míssil
    // olhando se o rastro já dissipou.
    std::vector<Vector2> trail;

private:
    float currentSpeed;
    float boostTimer;
    float travel;     // quanto andou no último frame, pra hitbox varrida
    float trailTimer;
};
