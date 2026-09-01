#pragma once
#include "raylib.h"
#include <vector>

// Define os tipos de explosão de forma genérica
enum class ExplosionType {
    TYPE_0 = 0,
    TYPE_1 = 1,
    TYPE_2 = 2,
    TYPE_3 = 3
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
    void Update(float deltaTime, float scrollSpeed = 0.0f);
    void Render() const;
    void Clear();

private:
    std::vector<ExplosionInstance> explosions;
    
    // Matriz de texturas: [ID da Explosão][Frame]
    // Preparado para até 4 tipos de explosões diferentes com 10 frames cada
    Texture2D expFrames[4][10] = {0};
    
    bool isLoaded = false;
};

