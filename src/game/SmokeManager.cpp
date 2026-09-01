#include "SmokeManager.h"

void SmokeManager::Initialize()
{
    if (isLoaded) return;
    
    // Zera os IDs antes de tentar carregar
    for (int i = 0; i < 7; i++) smokeFrames[i].id = 0;
    
    const char* smokePaths[] = {
        "assets/sprites/smokes/smoke1/",
        "../../assets/sprites/smokes/smoke1/",
        "../../../assets/sprites/smokes/smoke1/"
    };
    
    for (int p = 0; p < 3; p++)
    {
        if (smokeFrames[0].id == 0)
        {
            for (int f = 0; f < 7; f++)
            {
                smokeFrames[f] = LoadTexture(TextFormat("%ssmoke1_%d.png", smokePaths[p], f + 1));
            }
        }
    }
    
    isLoaded = true;
}

void SmokeManager::Render(Vector2 position, int frame) const
{
    if (!isLoaded || smokeFrames[0].id == 0) return;
    if (frame < 0 || frame >= 7) return;
    
    Texture2D sTex = smokeFrames[frame];
    float sW = (float)sTex.width;
    float sH = (float)sTex.height;
    Rectangle sSource = { 0.0f, 0.0f, sW, sH };
    
    // Offset da fumaça (-20 na vertical)
    float offsetX = 15.0f;
    float offsetY = -20.0f;
    
    Rectangle sDest = { position.x + offsetX, position.y + offsetY, sW, sH };
    Vector2 sOrigin = { sW / 2.0f, sH / 2.0f };
    
    DrawTexturePro(sTex, sSource, sDest, sOrigin, 0.0f, WHITE);
}

void SmokeManager::Unload()
{
    if (!isLoaded) return;
    for (int i = 0; i < 7; i++)
    {
        if (smokeFrames[i].id != 0)
        {
            UnloadTexture(smokeFrames[i]);
            smokeFrames[i].id = 0;
        }
    }
    isLoaded = false;
}
