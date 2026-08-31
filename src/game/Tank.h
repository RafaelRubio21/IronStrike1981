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
    
    void Destroy();

    bool isDestroyed;
    bool isActive;
    
    Vector2 position;

private:
    Vector2 velocity;
    float rotation;
    
    float scale; // Permite aumentar ou diminuir o tanque
    int currentFrame;
    float frameTimer;
};
