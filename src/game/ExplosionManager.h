#pragma once
#include "raylib.h"
#include <vector>

// Cada tipo é um efeito completo: animação, duração e som.
// A configuração de todos eles fica na tabela EXPLOSION_CONFIGS, no .cpp.
enum class ExplosionType {
    TYPE_0 = 0, // Explosão comum (construção derrubada, bala acertando o player)
    TYPE_1 = 1, // Explosão grande (o helicóptero batendo no chão)
    TYPE_2 = 2, // Faísca de bala batendo na blindagem
    TYPE_3 = 3  // Explosão de tanque
};

constexpr int EXPLOSION_TYPE_COUNT = 4;
constexpr int EXPLOSION_MAX_FRAMES = 10;

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

    // O som sai junto da animação: quem define é o tipo, não o chamador.
    // Um tipo sem som na tabela simplesmente não faz barulho.
    void Spawn(Vector2 position, ExplosionType type, float scale = 1.0f);

    void Update(float deltaTime, float scrollSpeed = 0.0f);
    void Render() const;
    void Clear();

    // Libera frames e sons. Precisa rodar antes de CloseWindow().
    void Unload();

private:
    std::vector<ExplosionInstance> explosions;

    // Matriz de texturas: [ID da Explosão][Frame]
    Texture2D expFrames[EXPLOSION_TYPE_COUNT][EXPLOSION_MAX_FRAMES] = {};

    // Um som por tipo. Tipos diferentes podem apontar para o mesmo arquivo:
    // nesse caso ele é carregado uma vez só e o handle é compartilhado.
    Sound sounds[EXPLOSION_TYPE_COUNT] = {};

    bool isLoaded = false;
};
