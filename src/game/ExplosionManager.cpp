#include "ExplosionManager.h"

void ExplosionManager::Initialize()
{
    explosions.clear();
    
    if (!isLoaded)
    {
        const char* rootPaths[] = {
            "assets/sprites/explosions/",
            "../../assets/sprites/explosions/",
            "../../../assets/sprites/explosions/"
        };
        
        // Vamos carregar a Explosion0 e Explosion1 (2 tipos)
        for (int t = 0; t < 2; t++)
        {
            for (int p = 0; p < 3; p++)
            {
                if (expFrames[t][0].id == 0)
                {
                    for (int f = 0; f < 10; f++)
                    {
                        expFrames[t][f] = LoadTexture(TextFormat("%sExplosion%d/f%d.png", rootPaths[p], t, f + 1));
                    }
                }
            }
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

void ExplosionManager::Update(float deltaTime)
{
    for (int i = 0; i < explosions.size(); i++)
    {
        explosions[i].frameTimer += deltaTime;
        if (explosions[i].frameTimer >= 0.05f) // 20 FPS
        {
            explosions[i].frameTimer = 0.0f;
            explosions[i].currentFrame++;
            if (explosions[i].currentFrame >= explosions[i].maxFrames)
            {
                explosions.erase(explosions.begin() + i);
                i--;
            }
        }
    }
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


