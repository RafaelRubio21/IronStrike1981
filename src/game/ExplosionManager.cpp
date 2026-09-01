#include "ExplosionManager.h"
#include <algorithm>

void ExplosionManager::Initialize()
{
    explosions.clear();
    
    if (!isLoaded)
    {
        // Vamos carregar a Explosion0 e Explosion1
        for (int t = 0; t < 2; t++)
        {
            for (int f = 0; f < 10; f++)
            {
                expFrames[t][f] = LoadTexture(TextFormat("assets/sprites/explosions/Explosion%d/f%d.png", t, f + 1));
            }
        }

        // Carrega o FireElement1 no slot TYPE_2 (que tem 7 frames apenas)
        for (int f = 0; f < 7; f++)
        {
            expFrames[2][f] = LoadTexture(TextFormat("assets/sprites/explosions/FireElement1/f%d.png", f + 1));
        }
        
        isLoaded = true;
    }
}

void ExplosionManager::Spawn(Vector2 position, ExplosionType type, float scale)
{
    ExplosionInstance ex;
    ex.position = position;
    ex.type = type;
    ex.currentFrame = 0;
    ex.frameTimer = 0.0f;
    ex.scale = scale;
    ex.maxFrames = 10; // Ambas tem 10 frames no momento
    
    explosions.push_back(ex);
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

    for (int t = 0; t < 4; t++)
    {
        for (int f = 0; f < 10; f++)
        {
            if (expFrames[t][f].id != 0)
            {
                UnloadTexture(expFrames[t][f]);
                expFrames[t][f].id = 0;
            }
        }
    }

    isLoaded = false;
}
