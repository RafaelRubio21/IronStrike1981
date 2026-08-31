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
    float speed;
    
    Texture2D sprite; // Corpo do helicoptero
    bool hasSprite;
    
    Texture2D rotorSprite; // Helice
    bool hasRotor;
    float rotorRotation; // Angulo atual de giro em graus
    float currentRotorSpeed; // Velocidade atual de giro
    float targetRotorSpeed; // Velocidade maxima
    
    float scale; // Escala do helicoptero (para decolagem)
};
