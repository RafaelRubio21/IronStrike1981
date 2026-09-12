#pragma once

#include "raylib.h"
#include "SoundPool.h"
#include <vector>

class Player
{
public:
    void Initialize(Vector2 startPos);
    void Update(float deltaTime);
    void DrawShadows() const; // Fase 1 do render
    void DrawBody() const;    // Fase 2 do render
    
    // Passa o retângulo de colisão do inimigo, retorna true e remove a bala se acertou
    bool CheckBulletHits(Rectangle targetRect);

    // Mesma ideia, mas pro tiro secundário (mais forte, mais lento) — quem
    // decide o dano é o Game, já que o valor difere por arma
    bool CheckSecondaryBulletHits(Rectangle targetRect);

    // Ponto de saída do míssil: alterna entre o pilone esquerdo e o direito
    // a cada disparo (mesmo lado que acabou de "descarregar" o míssil
    // decorativo — ver missileLastLaunchWasLeft).
    Vector2 GetMissileSpawnPos() const
    {
        const float sideX = missileLastLaunchWasLeft ? -missileWingOffsetX : missileWingOffsetX;
        return { position.x + sideX * scale, position.y + missileWingOffsetY * scale };
    }

    Rectangle GetHitbox() const;
    void TakeDamage(int damage);

    // Libera sprites e sons. Precisa rodar antes de CloseWindow().
    void Unload();
    
    Vector2 GetPosition() const { return position; }

    // Decolagem concluída: já pode voar, atirar e o mapa já pode rolar.
    // É a mesma condição que libera o controle do jogador.
    bool IsAirborne() const { return scale >= 1.0f; }

    // Fim de fase: tira o controle do jogador e leva o helicóptero até o
    // ponto de pouso, onde ele desce e desliga os motores.
    void StartLanding(Vector2 landingPos);
    bool IsLanding() const { return isLanding; }

    // Pousou e as hélices já pararam por completo
    bool HasShutDown() const { return enginesShutDown && currentRotorSpeed <= 0.0f; }
    
    int hp;
    int maxHp; // vida no Initialize(); é a referência 100% da barra do HUD
    float hitTimer;
    bool isDestroyed;
    bool justHitGround;

    // Fração de vida restante, de 0.0 a 1.0. Usa maxHp e não um valor fixo
    // porque o hp inicial pode mudar (é um valor de teste hoje).
    float GetHpRatio() const { return (maxHp > 0) ? ((float)hp / (float)maxHp) : 0.0f; }

    // --- Míssil (tecla D) ---
    // O Player só levanta a flag; quem cria e gerencia o míssil de verdade
    // (o PlayerMissile) é o Game, mesmo padrão do EnemyBullet do tanque.
    bool hasFiredMissile;
    int missileAmmo;
    int missileMaxAmmo;

private:
    // Desenha os 2 mísseis decorativos sob as asas. Chamado de dentro do
    // DrawBody(). A visibilidade é recalculada aqui a partir de missileAmmo
    // toda vez, não guardada num flag — assim, se a munição aumentar (um
    // pickup, por exemplo), os mísseis voltam a aparecer sozinhos.
    void DrawWingMissiles() const;

    // Mesma regra de visibilidade acima, mas usada também por DrawShadows()
    // — evita duplicar o cálculo em dois lugares.
    void GetWingMissileVisibility(bool& leftVisible, bool& rightVisible) const;

    Vector2 position;
    Vector2 velocity;     // Velocidade atual (Inercia)
    float acceleration;   // Forca do motor ao apertar a tecla
    float friction;       // Resistencia do ar (frenagem)
    
    Texture2D sprite; // Corpo do helicoptero
    Texture2D destroyedSprite;
    bool hasSprite;

    Texture2D rotorSprite; // Helice
    Texture2D destroyedRotorSprite;
    bool hasRotor;
    float rotorOffsetY; // Ajuste vertical da helice para o jogador alterar facilmente
    
    Texture2D machineGunSprite; // Animacao de Tiro
    bool hasMachineGun;
    bool isShooting;
    int mgCurrentFrame;
    float mgFrameTimer;
    float mgOffsetY; // Distancia para o nariz do helicoptero
    
    std::vector<Vector2> bullets; // Municao ativa na tela
    float bulletSpeed;
    float mgFireRate; // Frequencia dos tiros (em segundos)

    // Quanto a bala andou no ultimo frame. A hitbox dela cobre esse trecho
    // inteiro, senao num quadro lento ela pula por cima do alvo.
    float bulletTravel;

    // --- Tiro secundário: mais forte e mais lento que a metralhadora ---
    // Sem sprite de fogo próprio ainda (usa só o projétil, sem animação no bico).
    bool isShootingSecondary;
    std::vector<Vector2> secondaryBullets;
    float secondaryBulletSpeed;
    float secondaryFireRate;
    float secondaryFireTimer;
    float secondaryBulletTravel; // mesmo raciocínio do bulletTravel, pro tiro secundário
    SoundPool mgSecondaryShootSound;

    float missileCooldownTimer; // conta regressiva até poder disparar outro míssil
    Sound missileLaunchSound;   // toca uma vez por míssil (cadência baixa, não precisa de SoundPool)

    // Alterna a cada disparo: true = a próxima saída é pelo pilone
    // esquerdo. missileLastLaunchWasLeft guarda de qual lado saiu o tiro
    // que acabou de disparar (é o valor de missileNextLaunchIsLeft ANTES de
    // alternar), pro GetMissileSpawnPos() e o desenho ficarem sincronizados.
    bool missileNextLaunchIsLeft;
    bool missileLastLaunchWasLeft;
    float missileWingOffsetX; // distância horizontal do pilone até o centro do helicóptero
    float missileWingOffsetY; // distância vertical do pilone até o centro (a asa fica um pouco acima do centro)

    Texture2D missileWingSprite; // versão decorativa presa na asa (não voa)
    bool hasMissileWingSprite;

    float rotorRotation; // Angulo atual de giro em graus
    float currentRotorSpeed; // Velocidade atual de giro
    float targetRotorSpeed; // Velocidade maxima
    
    float scale; // Escala do helicoptero (para decolagem)
    
    // Motor de Som do Helicoptero
    Sound engineStartingSound;
    Music engineLoopMusic;
    bool engineLoopActive;
    float engineStartDelayTimer;
    
    SoundPool mgShootSound; // varias vozes: 12 tiros por segundo se cortavam
    Sound mgFinalShotSound;
    bool wasShooting;

    // Pouso de fim de fase
    bool isLanding;
    Vector2 landingTarget;
    bool enginesShutDown;      // já tocou o shutdown e a hélice está parando
    float rotorShutdownRate;   // graus/s² para casar a parada com o som
    Sound engineShutdownSound;
};
