#pragma once
#include "raylib.h"
#include <vector>

class EnemyBullet
{
public:
    // Raio do desenho da bala. Fica aqui porque a colisão usa o mesmo valor.
    static constexpr float RADIUS = 7.0f;

    void Initialize(Vector2 startPos, Vector2 forwardDir, float speed);
    void Update(float deltaTime);
    void Render() const;
    void OnHit();
    
    Vector2 position;
    Vector2 velocity;
    bool active;
    
    std::vector<Vector2> trail;
    float trailTimer;
};
