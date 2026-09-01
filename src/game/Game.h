#pragma once

#include "raylib.h"
#include "Player.h"
#include "Tank.h"
#include "ExplosionManager.h"
#include "SmokeManager.h"
#include "EnemyBullet.h"
#include <vector>
#include <memory>



class Game
{
public:
    void Initialize();
    void Update(float deltaTime);
    void Render();
    void Shutdown();

private:
    Player player;
    
    std::vector<std::unique_ptr<EnemyBase>> enemies;
    ExplosionManager explosionManager;
    SmokeManager smokeManager;
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


