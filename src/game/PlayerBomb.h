#pragma once
#include "raylib.h"

// Bomba do player (tecla F). Diferente do míssil, não é propulsionada: é
// só jogada. Herda a inércia do helicóptero no instante da soltura e vai
// perdendo essa velocidade por atrito com o ar enquanto cai — por isso
// não desce em linha 100% reta.
//
// A "altura" dela é simulada por um temporizador (fallTimer), não por
// posição: a sombra começa afastada do sprite (mesma distância que o
// helicóptero usa voando em altitude máxima) e vai se aproximando dele
// conforme a bomba se aproxima do chão, até coincidir no instante do
// impacto — é isso que dá a sensação de queda num jogo 2D top-down.
//
// Quem é dono da lista de bombas é o Game, mesmo padrão do EnemyBullet e
// do PlayerMissile. O Player só levanta a flag hasDroppedBomb.
class PlayerBomb
{
public:
    void Initialize(Vector2 startPos, Vector2 initialVelocity);
    void Update(float deltaTime);
    void Render() const;
    void DrawShadows() const;

    // true quando termina de cair — o Game decide o que acontece
    // (explosão, remoção da lista) ao ver isso true.
    bool HasLanded() const;

    Vector2 GetPosition() const { return position; }

    // Libera a textura compartilhada entre todas as bombas. Precisa rodar
    // antes de CloseWindow().
    static void UnloadSharedAssets();

private:
    Vector2 position;
    Vector2 velocity;
    float fallTimer;
};
