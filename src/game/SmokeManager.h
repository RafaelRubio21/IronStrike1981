#pragma once
#include "raylib.h"

class SmokeManager
{
public:
    void Initialize();
    void Render(Vector2 position, int frame) const;
    void Unload();

private:
    Texture2D smokeFrames[7];
    bool isLoaded = false;
};
