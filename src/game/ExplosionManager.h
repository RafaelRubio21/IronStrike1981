#pragma once
#include "raylib.h"
#include <vector>

// Define os tipos de explosão que você pode ter
enum class ExplosionType {
    TANK_EXPLOSION,
    HIT_EXPLOSION,
    PLAYER_EXPLOSION
};

// Guarda os dados de uma única explosão ocorrendo na tela
struct ExplosionInstance {
    Vector2 position;
    ExplosionType type;
    int currentFrame;
    float frameTimer;
    float scale;
    int maxFrames; // Quantos quadros essa explosão específica tem
};

class ExplosionManager {
public:
    void Initialize();
    void Spawn(Vector2 position, ExplosionType type, float scale = 1.0f);
    void Update(float deltaTime);
    void Render() const;
    void Clear();

private:
    std::vector<ExplosionInstance> explosions;
    
    // Armazenamento das texturas em memória (carregadas apenas 1 vez)
    Texture2D tankExpFrames[10] = {0};
    Texture2D hitExpFrames[10] = {0}; // Pode ser a mesma por enquanto, mas preparado pra outra
    Texture2D playerExpFrames[10] = {0}; // Texturas do Explosion3
    
    bool isLoaded = false;
};
