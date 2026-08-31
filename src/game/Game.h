#pragma once

#include "raylib.h"
#include "Player.h"

class Game
{
public:
    void Initialize();
    void Update(float deltaTime);
    void Render();

private:
    Player player;
    Camera2D camera;
    float levelScrollY; // O mapa desce, dando a ilusao de voar para cima
    float scrollSpeed;
    
    Music bgMusic; // Musica de Fundo
    float bgMusicVolume;
    float bgMusicTargetVolume;
    
    RenderTexture2D globalShadowTarget; // O grande canvas global de sombras unificadas!
};
