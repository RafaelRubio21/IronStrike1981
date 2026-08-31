#pragma once

#include "raylib.h"
#include "Player.h"
#include "Tank.h"
#include <vector>

class Game
{
public:
    void Initialize();
    void Update(float deltaTime);
    void Render();

private:
    Player player;
    
    std::vector<Tank> tanks;
    float tankSpawnTimer;
    
    Camera2D camera;
    float levelScrollY; // O mapa desce, dando a ilusao de voar para cima
    float scrollSpeed;
    
    Music bgMusic; // Musica de Fundo
    float bgMusicVolume;
    float bgMusicTargetVolume;
    
    RenderTexture2D globalShadowTarget; // O grande canvas global de sombras unificadas!
};
