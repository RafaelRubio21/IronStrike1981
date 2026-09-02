#pragma once
#include "raylib.h"

#include <vector>

#include "EnemyBase.h"

class Tank : public EnemyBase
{
public:
    void Initialize(Vector2 startPos, int spawnDirection, int tankType = 0, std::vector<Vector2> path = {}, Rectangle patrolArea = {0,0,0,0}, EnemyStats mapStats = {}); 
    void Update(float deltaTime, Vector2 playerPos, bool playerDestroyed, float scrollSpeed = 0.0f) override;
    void DrawShadows() const override;
    void DrawBody() const override;
    
    void Destroy() override;
    Rectangle GetHitbox() const override;

    // Libera as texturas e sons compartilhados por TODOS os tanques.
    // Precisa rodar antes de CloseWindow(), enquanto o contexto gráfico ainda existe.
    static void UnloadSharedAssets();

    // Status individuais do tanque (podem variar por tipo)
    float currentSpeedMult;
    float turretSpeed;
    float hullTurnSpeed;  // Graus por segundo que o chassi consegue girar
    float cannonOffsetY;
    
private:
    std::vector<Vector2> waypoints;
    int currentWaypoint;
    int pathDirection;
    Rectangle patrolBounds;
    bool isPatrolling;
    int currentFrame;
    float frameTimer;
    
    int cannonFrame;
    float cannonAnimTimer;
    bool isCannonShooting;
    float turretAngularVel; // Velocidade angular atual da torreta (inércia)
};









