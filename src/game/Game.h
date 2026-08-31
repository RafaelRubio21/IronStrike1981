#pragma once

#include "raylib.h"
#include "Player.h"
#include "Tank.h"
#include <vector>

struct Explosion {
    Vector2 position;
    int currentFrame;
    float frameTimer;
    float scale;
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
    std::vector<Explosion> explosions;
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
