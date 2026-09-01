#pragma once
#include "raylib.h"
#include <vector>

class EnemyBullet
{
public:
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
