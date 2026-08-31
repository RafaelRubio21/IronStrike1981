#pragma once
#include "raylib.h"

class Tank
{
public:
    void Initialize(Vector2 startPos, int spawnDirection); 
    void Update(float deltaTime);
    void DrawShadows() const;
    void DrawBody() const;
    
    Rectangle GetHitbox() const;
    
    void TakeDamage(int damage);
    void Destroy();

    bool isDestroyed;
    bool isActive;
    
    int hp;
    float hitTimer; // Para piscar a tela quando toma tiro
    
    Vector2 position;

private:
    Vector2 velocity;
    float rotation;
    
    float scale; // Permite aumentar ou diminuir o tanque
    int currentFrame;
    float frameTimer;
};
