#pragma once

#include "raylib.h"
#include "Player.h"
#include "Tank.h"
#include "ExplosionManager.h"
#include "SmokeManager.h"
#include "EnemyBullet.h"
#include "map/MapManager.h"
#include "SoundPool.h"
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
    // Joga o canvas de sombras na tela com a transparência final
    void StampShadows();

    // Escolhe um dos sons de metal e toca. Usado quando a bala bate mas o
    // alvo (tanque ou construção) aguenta o tranco.
    void PlayImpactSound();

    // Barra de vida, pontuação e progresso da fase, por cima de tudo
    void RenderHUD() const;

    Player player;
    
    std::vector<std::unique_ptr<EnemyBase>> enemies;
    ExplosionManager explosionManager;
    SmokeManager smokeManager;
    MapManager mapManager;
    std::vector<EnemyBullet> enemyBullets;

    float scrollSpeed;

    // Pontuação: soma ao destruir um tanque ou derrubar uma construção
    int score;
    
    Music bgMusic; // Musica de Fundo
    float bgMusicVolume;
    float bgMusicTargetVolume;
    
    static constexpr int IMPACT_SOUND_COUNT = 5;
    SoundPool impactSounds[IMPACT_SOUND_COUNT]; // Sons genericos de metal
    
    RenderTexture2D globalShadowTarget; // O grande canvas global de sombras unificadas!
};



