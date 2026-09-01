#pragma once

// Constantes globais do jogo.
// Antes esses números estavam repetidos em main.cpp, Game.cpp, Player.cpp,
// MapManager.cpp e EnemyBase.h — mudar a resolução quebrava o culling,
// o clamp do player e o render target de sombras.
namespace Config
{
    constexpr int SCREEN_WIDTH  = 1024;
    constexpr int SCREEN_HEIGHT = 768;

    // Folga fora da tela antes de um objeto ser descartado
    constexpr float CULL_MARGIN = 300.0f;
}
