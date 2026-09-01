#include "EnemyBullet.h"

void EnemyBullet::Initialize(Vector2 startPos, Vector2 forwardDir, float speed)
{
    position = startPos;
    velocity = { forwardDir.x * speed, forwardDir.y * speed };
    active = true;
    trailTimer = 0.0f;
    trail.clear();
}

void EnemyBullet::Update(float deltaTime)
{
    if (active)
    {
        position.x += velocity.x * deltaTime;
        position.y += velocity.y * deltaTime;
        
        trailTimer += deltaTime;
        if (trailTimer >= 0.03f)
        {
            trailTimer = 0.0f;
            trail.push_back(position);
            if (trail.size() > 10) trail.erase(trail.begin());
        }
    }
    else
    {
        // Bala já explodiu, dissipar a fumaça
        trailTimer += deltaTime;
        if (trailTimer >= 0.03f && !trail.empty())
        {
            trailTimer = 0.0f;
            trail.erase(trail.begin());
        }
    }
}

void EnemyBullet::Render() const
{
    // Rastro (Fumaça dissipando)
    for (int i = 0; i < (int)trail.size(); i++)
    {
        float alpha = (float)i / (float)trail.size(); // Os mais recentes são mais fortes
        float size = alpha * 4.0f;
        DrawCircleV(trail[i], size, { 80, 80, 80, (unsigned char)(alpha * 200) });
        DrawCircleV(trail[i], size * 0.5f, { 255, 100, 0, (unsigned char)(alpha * 150) }); // Centro laranjinha do rastro
    }
    
    if (active)
    {
        // Centro da Bala (Plasma/Fogo incandescente)
        DrawCircleV(position, 7.0f, { 255, 50, 0, 255 }); // Borda laranja escuro
        DrawCircleV(position, 4.0f, ORANGE);              // Laranja
        DrawCircleV(position, 2.0f, WHITE);               // Centro quente
    }
}

void EnemyBullet::OnHit()
{
    active = false;
}
