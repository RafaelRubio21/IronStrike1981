#include "ExplosionManager.h"
#include <algorithm>
#include <cstring>

// =================================================================
// A IDENTIDADE DE CADA TIPO DE EXPLOSÃO
// Imagem, duração e som no mesmo lugar. Para trocar o som de um tipo,
// dar som a um que ainda não tem, ou criar um efeito novo, é aqui.
// soundPath nullptr = explosão muda.
// =================================================================
struct ExplosionConfig {
    const char* folder;      // pasta em assets/sprites/explosions/
    int frameCount;          // quantos frames a animação REALMENTE tem
    const char* soundPath;   // caminho dentro de assets/audio/
    float volume;            // 0.0 a 1.0
};

static const ExplosionConfig EXPLOSION_CONFIGS[EXPLOSION_TYPE_COUNT] = {
    // pasta          frames  som                          volume
    { "Explosion0",     10,   "explosions/explosion1.ogg",  0.6f }, // TYPE_0: comum
    { "Explosion1",     10,   "explosions/explosion1.ogg",  0.8f }, // TYPE_1: player caindo
    { "FireElement1",    7,   nullptr,                      0.0f }, // TYPE_2: faísca (o impacto metálico é do Game)
    { "Explosion0",     10,   "tank/exploding.ogg",         0.5f }, // TYPE_3: tanque
};

void ExplosionManager::Initialize()
{
    explosions.clear();

    if (isLoaded) return;

    for (int t = 0; t < EXPLOSION_TYPE_COUNT; t++)
    {
        const ExplosionConfig& cfg = EXPLOSION_CONFIGS[t];

        if (cfg.folder != nullptr)
        {
            const int total = (cfg.frameCount < EXPLOSION_MAX_FRAMES) ? cfg.frameCount : EXPLOSION_MAX_FRAMES;
            for (int f = 0; f < total; f++)
            {
                expFrames[t][f] = LoadTexture(TextFormat("assets/sprites/explosions/%s/f%d.png", cfg.folder, f + 1));
            }
        }

        if (cfg.soundPath == nullptr) continue;

        // Dois tipos podem usar o mesmo arquivo de som: carrega uma vez só e
        // compartilha o handle, senão o mesmo .ogg ficaria duplicado na memória.
        int fonte = -1;
        for (int k = 0; k < t; k++)
        {
            if (EXPLOSION_CONFIGS[k].soundPath != nullptr &&
                strcmp(EXPLOSION_CONFIGS[k].soundPath, cfg.soundPath) == 0)
            {
                fonte = k;
                break;
            }
        }

        sounds[t] = (fonte >= 0) ? sounds[fonte]
                                 : LoadSound(TextFormat("assets/audio/%s", cfg.soundPath));
    }

    isLoaded = true;
}

void ExplosionManager::Spawn(Vector2 position, ExplosionType type, float scale)
{
    const int t = (int)type;
    if (t < 0 || t >= EXPLOSION_TYPE_COUNT) return;

    ExplosionInstance ex;
    ex.position = position;
    ex.type = type;
    ex.currentFrame = 0;
    ex.frameTimer = 0.0f;
    ex.scale = scale;
    ex.maxFrames = EXPLOSION_CONFIGS[t].frameCount;

    explosions.push_back(ex);

    // O volume é ajustado na hora de tocar, e não no carregamento, porque dois
    // tipos podem dividir o mesmo Sound com volumes diferentes.
    if (sounds[t].frameCount != 0)
    {
        SetSoundVolume(sounds[t], EXPLOSION_CONFIGS[t].volume);
        PlaySound(sounds[t]);
    }
}

void ExplosionManager::Update(float deltaTime, float scrollSpeed)
{
    for (auto& ex : explosions)
    {
        ex.frameTimer += deltaTime;
        ex.position.y += scrollSpeed * deltaTime;
        if (ex.frameTimer >= 0.05f) // 20 FPS
        {
            ex.frameTimer = 0.0f;
            ex.currentFrame++;
        }
    }

    // Remove as que terminaram a animação
    explosions.erase(std::remove_if(explosions.begin(), explosions.end(),
        [](const ExplosionInstance& ex) { return ex.currentFrame >= ex.maxFrames; }), explosions.end());
}

void ExplosionManager::Render() const
{
    for (const auto& ex : explosions)
    {
        Texture2D tex = expFrames[(int)ex.type][ex.currentFrame];

        if (tex.id != 0)
        {
            float w = (float)tex.width * ex.scale;
            float h = (float)tex.height * ex.scale;

            Rectangle source = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
            Rectangle dest = { ex.position.x, ex.position.y, w, h };
            Vector2 origin = { w / 2.0f, h / 2.0f };

            DrawTexturePro(tex, source, dest, origin, 0.0f, WHITE);
        }
    }
}

void ExplosionManager::Clear()
{
    explosions.clear();
}

void ExplosionManager::Unload()
{
    explosions.clear();

    for (int t = 0; t < EXPLOSION_TYPE_COUNT; t++)
    {
        for (int f = 0; f < EXPLOSION_MAX_FRAMES; f++)
        {
            if (expFrames[t][f].id != 0)
            {
                UnloadTexture(expFrames[t][f]);
                expFrames[t][f].id = 0;
            }
        }
    }

    // Tipos que compartilham o mesmo arquivo compartilham o handle: descarrega
    // uma vez e zera todos os slots que apontavam para ele.
    for (int t = 0; t < EXPLOSION_TYPE_COUNT; t++)
    {
        if (sounds[t].frameCount == 0) continue;

        Sound s = sounds[t];
        UnloadSound(s);

        for (int k = t; k < EXPLOSION_TYPE_COUNT; k++)
        {
            if (sounds[k].stream.buffer == s.stream.buffer) sounds[k] = {};
        }
    }

    isLoaded = false;
}
