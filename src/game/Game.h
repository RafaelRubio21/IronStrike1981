#pragma once

#include "raylib.h"
#include "Player.h"
#include "Tank.h"
#include "ExplosionManager.h"
#include <vector>

struct EnemyBullet {
    Vector2 position;
    Vector2 velocity;
    bool active;
    
    std::vector<Vector2> trail;
    float trailTimer;
};

class Game
{
public:
    void Initialize();
    void Update(float deltaTime);
    void Render();
    void Shutdown();

private:
    Player player;
    
    std::vector<Tank> tanks;
    ExplosionManager explosionManager;
    std::vector<EnemyBullet> enemyBullets;
    float tankSpawnTimer;
    
    Camera2D camera;
    float levelScrollY; // O mapa desce, dando a ilusao de voar para cima
    float scrollSpeed;
    
    Music bgMusic; // Musica de Fundo
    float bgMusicVolume;
    float bgMusicTargetVolume;
    
    Sound impactSounds[5]; // Sons genericos de metal
    
    RenderTexture2D globalShadowTarget; // O grande canvas global de sombras unificadas!
};

