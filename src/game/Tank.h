#pragma once
#include "raylib.h"

#include <vector>

struct TrackMark {
    Vector2 position;
    float rotation;
    float lifeTime;
};

struct DustParticle {
    Vector2 position;
    Vector2 velocity;
    float lifeTime;
    float maxLife;
    float radius;
};

class Tank
{
public:
    void Initialize(Vector2 startPos, int spawnDirection, int tankType = 0); 
    void Update(float deltaTime, Vector2 playerPos, bool playerDestroyed);
    void DrawTracksAndDust() const;
    void DrawShadows() const;
    void DrawBody() const;
    
    Rectangle GetHitbox() const;
    
    void TakeDamage(int damage);
    void Destroy();

    bool isDestroyed;
    bool isActive;
    bool hasFired;
    
    int type; // Guarda o modelo do tanque
    int hp;
    float hitTimer; // Para piscar a tela quando toma tiro
    
    Vector2 position;
    
    // Status individuais do tanque (podem variar por tipo)
    float speed;
    float currentSpeedMult;
    float turretSpeed;
    float cannonOffsetY;
    float fireOffsetY;
    float cannonRotation;
    
    int smokeFrame;
    float smokeAnimTimer;
    
    std::vector<TrackMark> tracks;
    float trackSpawnTimer;
    
    std::vector<DustParticle> dustParticles;
    float dustSpawnTimer;

private:
    Vector2 velocity;
    float rotation;
    
    float scale; // Permite aumentar ou diminuir o tanque
    int currentFrame;
    float frameTimer;
    
    int cannonFrame;
    float cannonAnimTimer;
    bool isCannonShooting;
    float shootCooldown;
};


