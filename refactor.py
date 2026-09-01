import os

# Game.h
with open('src/game/Game.h', 'r', encoding='utf-8') as f:
    h_content = f.read()

h_content = h_content.replace('#include "ExplosionManager.h"', '#include "ExplosionManager.h"\n#include "SmokeManager.h"\n#include "EnemyBullet.h"')
h_content = h_content.replace('''struct EnemyBullet {
    Vector2 position;
    Vector2 velocity;
    bool active;
    
    std::vector<Vector2> trail;
    float trailTimer;
};''', '')
h_content = h_content.replace('ExplosionManager explosionManager;', 'ExplosionManager explosionManager;\n    SmokeManager smokeManager;')

with open('src/game/Game.h', 'w', encoding='utf-8') as f:
    f.write(h_content)


# Game.cpp
with open('src/game/Game.cpp', 'r', encoding='utf-8') as f:
    c_content = f.read()

c_content = c_content.replace('explosionManager.Initialize();', 'explosionManager.Initialize();\n    smokeManager.Initialize();')

smoke_vars = '''static Texture2D smokeFrames[7] = {0};
static bool smokeLoaded = false;
static float SMOKE_OFFSET_X = 15.0f;   // Ajuste da fumaça na horizontal
static float SMOKE_OFFSET_Y = -20.0f; // Ajuste da fumaça na vertical (-10 puxa pra cima do chassi)\n'''
c_content = c_content.replace(smoke_vars, '')

smoke_load = '''    // Carrega a Animação de Fumaça (smoke1_1 até smoke1_7)
    if (!smokeLoaded)
    {
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
        smokeLoaded = true;
    }\n'''
c_content = c_content.replace(smoke_load, '')

bullet_spawn = '''            float offset = std::abs(enemies[i]->fireOffsetY);
            bullet.position.x = enemies[i]->position.x + (forward.x * offset);
            bullet.position.y = enemies[i]->position.y + (forward.y * offset);
            
            // Velocidade devagar (250 pixels por segundo)
            bullet.velocity.x = forward.x * 250.0f;
            bullet.velocity.y = forward.y * 250.0f;
            
            bullet.active = true;
            bullet.trailTimer = 0.0f;
            enemyBullets.push_back(bullet);'''
new_bullet_spawn = '''            float offset = std::abs(enemies[i]->fireOffsetY);
            Vector2 startPos = { enemies[i]->position.x + (forward.x * offset), enemies[i]->position.y + (forward.y * offset) };
            EnemyBullet bullet;
            bullet.Initialize(startPos, forward, 250.0f);
            enemyBullets.push_back(bullet);'''
c_content = c_content.replace(bullet_spawn, new_bullet_spawn)

bullet_update = '''        if (b.active)
        {
            b.position.x += b.velocity.x * deltaTime;
            b.position.y += b.velocity.y * deltaTime;
            
            // Criação do Rastro (Fumaça)
            b.trailTimer += deltaTime;
            if (b.trailTimer >= 0.03f) // A cada 0.03s deixa um rastro
            {
                b.trailTimer = 0.0f;
                b.trail.push_back(b.position);
                if (b.trail.size() > 10) b.trail.erase(b.trail.begin());
            }
            
            // Colisão com o Jogador
            if (!player.isDestroyed && CheckCollisionPointRec(b.position, player.GetHitbox()))
            {
                player.TakeDamage(); // Helicóptero perde vida
                b.active = false;    // Bala some
            }
        }
        else
        {
            // Bala já explodiu, dissipar a fumaça
            b.trailTimer += deltaTime;
            if (b.trailTimer >= 0.03f && !b.trail.empty())
            {
                b.trailTimer = 0.0f;
                b.trail.erase(b.trail.begin());
            }
        }
        
        // Destrói se sair muito da tela ou se já dissipou toda a fumaça após bater
        if (b.position.x < -200 || b.position.x > 1200 || b.position.y < -200 || b.position.y > 1000 || (!b.active && b.trail.empty()))
        {
            enemyBullets.erase(enemyBullets.begin() + i);
            i--;
        }'''
new_bullet_update = '''        b.Update(deltaTime);
        
        if (b.active && !player.isDestroyed && CheckCollisionPointRec(b.position, player.GetHitbox()))
        {
            player.TakeDamage();
            b.OnHit();
        }
        
        if (b.position.x < -200 || b.position.x > 1200 || b.position.y < -200 || b.position.y > 1000 || (!b.active && b.trail.empty()))
        {
            enemyBullets.erase(enemyBullets.begin() + i);
            i--;
        }'''
c_content = c_content.replace(bullet_update, new_bullet_update)

smoke_render = '''    // ETAPA 5: DESENHAR FUMAÇAS E EXPLOSÕES POR CIMA DE TUDO
    for (const auto& e : enemies)
    {
        if (e->isDestroyed && smokeFrames[0].id != 0)
        {
            Texture2D sTex = smokeFrames[e->smokeFrame];
            float sW = (float)sTex.width;
            float sH = (float)sTex.height;
            Rectangle sSource = { 0.0f, 0.0f, (float)sTex.width, (float)sTex.height };
            Rectangle sDest = { e->position.x + SMOKE_OFFSET_X, e->position.y + SMOKE_OFFSET_Y, sW, sH };
            Vector2 sOrigin = { sW / 2.0f, sH / 2.0f };
            DrawTexturePro(sTex, sSource, sDest, sOrigin, 0.0f, WHITE);
        }
    }'''
new_smoke_render = '''    // ETAPA 5: DESENHAR FUMAÇAS E EXPLOSÕES POR CIMA DE TUDO
    for (const auto& e : enemies)
    {
        if (e->isDestroyed) smokeManager.Render(e->position, e->smokeFrame);
    }'''
c_content = c_content.replace(smoke_render, new_smoke_render)

bullet_render = '''    // Tiros Inimigos
    for (const auto& b : enemyBullets)
    {
        // Rastro
        for (int i = 0; i < (int)b.trail.size(); i++)
        {
            float alpha = (float)i / (float)b.trail.size();
            float size = alpha * 4.0f;
            DrawCircleV(b.trail[i], size, { 80, 80, 80, (unsigned char)(alpha * 200) });
            DrawCircleV(b.trail[i], size * 0.5f, { 255, 100, 0, (unsigned char)(alpha * 150) });
        }
        
        if (b.active)
        {
            DrawCircleV(b.position, 7.0f, { 255, 50, 0, 255 });
            DrawCircleV(b.position, 4.0f, ORANGE);
            DrawCircleV(b.position, 2.0f, WHITE);
        }
    }'''
new_bullet_render = '''    // Tiros Inimigos
    for (const auto& b : enemyBullets)
    {
        b.Render();
    }'''
c_content = c_content.replace(bullet_render, new_bullet_render)

with open('src/game/Game.cpp', 'w', encoding='utf-8') as f:
    f.write(c_content)
