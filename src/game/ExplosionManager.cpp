#include "ExplosionManager.h"

void ExplosionManager::Initialize()
{
    explosions.clear();
    
    if (!isLoaded)
    {
        const char* expPaths[] = {
            "assets/sprites/explosions/explosion2/",
            "../../assets/sprites/explosions/explosion2/",
            "../../../assets/sprites/explosions/explosion2/"
        };
        
        for (int p = 0; p < 3; p++)
        {
            if (tankExpFrames[0].id == 0)
            {
                for (int f = 0; f < 10; f++)
                {
                    tankExpFrames[f] = LoadTexture(TextFormat("%sExplosion2_%d.png", expPaths[p], f + 1));
                    hitExpFrames[f] = tankExpFrames[f]; // Reusa Explosion2

                    // Carrega Explosion3 para o player (assumindo a pasta Explosion3 nas mesmas raizes)
                    const char* exp3Paths[] = {
                        "assets/sprites/explosions/Explosion3/",
                        "../../assets/sprites/explosions/Explosion3/",
                        "../../../assets/sprites/explosions/Explosion3/"
                    };
                    playerExpFrames[f] = LoadTexture(TextFormat("%sExplosion3_%d.png", exp3Paths[p], f + 1));
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
        Texture2D tex;
        if (ex.type == ExplosionType::PLAYER_EXPLOSION) tex = playerExpFrames[ex.currentFrame];
        else if (ex.type == ExplosionType::TANK_EXPLOSION) tex = tankExpFrames[ex.currentFrame];
        else tex = hitExpFrames[ex.currentFrame];
        
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

