#pragma once

#include "raylib.h"
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
    float hitTimer;
    bool isDestroyed;
    bool justHitGround;

private:
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
    
    float rotorRotation; // Angulo atual de giro em graus
    float currentRotorSpeed; // Velocidade atual de giro
    float targetRotorSpeed; // Velocidade maxima
    
    float scale; // Escala do helicoptero (para decolagem)
    
    // Motor de Som do Helicoptero
    Sound engineStartingSound;
    Music engineLoopMusic;
    bool engineLoopActive;
    float engineStartDelayTimer;
    
    Sound mgShootSound;
    Sound mgFinalShotSound;
    bool wasShooting;

    // Pouso de fim de fase
    bool isLanding;
    Vector2 landingTarget;
    bool enginesShutDown;      // já tocou o shutdown e a hélice está parando
    float rotorShutdownRate;   // graus/s² para casar a parada com o som
    Sound engineShutdownSound;
};
