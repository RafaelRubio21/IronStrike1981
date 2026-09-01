import re

with open('src/game/Game.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

old_init = '''            float screenY = spawn.y - mapManager.GetScrollY();
            float screenX = spawn.x;
            
            t->Initialize({ screenX, screenY }, spawn.direction, 0, spawn.path); 
            enemies.push_back(std::move(t));'''

new_init = '''            float screenY = spawn.y - mapManager.GetScrollY();
            float screenX = spawn.x;
            
            // A Rota está em World Coordinates. Precisamos converter TUDO para Screen Coordinates!
            std::vector<Vector2> screenPath;
            for (const auto& wp : spawn.path) {
                screenPath.push_back({ wp.x, wp.y - mapManager.GetScrollY() });
            }
            
            t->Initialize({ screenX, screenY }, spawn.direction, 0, screenPath); 
            enemies.push_back(std::move(t));'''

content = content.replace(old_init, new_init)

with open('src/game/Game.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
