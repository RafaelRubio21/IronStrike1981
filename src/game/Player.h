#pragma once

#include "raylib.h"

class Player
{
public:
    void Initialize(Vector2 startPos);
    void Update(float deltaTime);
    void Render() const;

private:
    Vector2 position;
    Vector2 velocity;     // Velocidade atual (Inercia)
    float acceleration;   // Forca do motor ao apertar a tecla
    float friction;       // Resistencia do ar (frenagem)
    
    Texture2D sprite; // Corpo do helicoptero
    bool hasSprite;
    
    Texture2D rotorSprite; // Helice
    bool hasRotor;
    float rotorRotation; // Angulo atual de giro em graus
    float currentRotorSpeed; // Velocidade atual de giro
    float targetRotorSpeed; // Velocidade maxima
    
    float scale; // Escala do helicoptero (para decolagem)
    
    // Motor de Som do Helicoptero
    Sound engineStartingSound;
    Music engineLoopMusic;
    bool engineLoopActive;
    float engineStartDelayTimer;
};
