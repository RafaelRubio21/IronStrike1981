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

private:
    Vector2 position;
    Vector2 velocity;     // Velocidade atual (Inercia)
    float acceleration;   // Forca do motor ao apertar a tecla
    float friction;       // Resistencia do ar (frenagem)
    
    Texture2D sprite; // Corpo do helicoptero
    bool hasSprite;

    Texture2D rotorSprite; // Helice
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
};
