import re

with open('src/game/Game.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

old_block = '''    for (int i = 0; i < enemies.size(); i++)
    {
        enemies[i]->Update(deltaTime, player.GetPosition(), player.isDestroyed);'''

new_block = '''    for (int i = 0; i < enemies.size(); i++)
    {
        // Aplica o movimento da câmera sobre todos os inimigos (Ilusão de movimento)
        enemies[i]->position.y += scrollSpeed * deltaTime;
        
        enemies[i]->Update(deltaTime, player.GetPosition(), player.isDestroyed);'''

content = content.replace(old_block, new_block)

with open('src/game/Game.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
