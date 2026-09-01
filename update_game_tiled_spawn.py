import re

with open('src/game/Game.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

old_spawner_comment = '// Gerador de Tanques desativado (aguardando spawns via Tiled map!)'

new_spawner = '''// Spawn de Inimigos via Mapa do Tiled
    std::vector<EnemySpawnData> newSpawns = mapManager.PopReadySpawns();
    for (const auto& spawn : newSpawns)
    {
        if (spawn.type == "Tank")
        {
            auto t = std::make_unique<Tank>();
            
            // spawn.x e spawn.y estão em coordenadas mundiais (World Y).
            // Precisamos converter para coordenadas de Tela (Screen Y)!
            float screenY = spawn.y - mapManager.GetScrollY();
            float screenX = spawn.x;
            
            t->Initialize({ screenX, screenY }, spawn.direction, 0); 
            enemies.push_back(std::move(t));
        }
    }'''

content = content.replace(old_spawner_comment, new_spawner)

with open('src/game/Game.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
