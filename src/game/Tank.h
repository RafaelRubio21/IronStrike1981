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

#include "EnemyBase.h"

class Tank : public EnemyBase
{
public:
    void Initialize(Vector2 startPos, int spawnDirection, int tankType = 0); 
    void Update(float deltaTime, Vector2 playerPos, bool playerDestroyed) override;
    void DrawTracksAndDust() const;
    void DrawShadows() const override;
    void DrawBody() const override;
    
    Rectangle GetHitbox() const override;
    
    void Destroy() override;

    // Status individuais do tanque (podem variar por tipo)
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
    int currentFrame;
    float frameTimer;
    
    int cannonFrame;
    float cannonAnimTimer;
    bool isCannonShooting;
    float turretAngularVel; // Velocidade angular atual da torreta (inércia)
};
