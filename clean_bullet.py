import sys

with open('src/game/Game.h', 'r', encoding='utf-8') as f:
    content = f.read()

# Replace struct EnemyBullet with include
struct_str = '''struct EnemyBullet {
    Vector2 position;
    Vector2 velocity;
    bool active;
    
    std::vector<Vector2> trail;
    float trailTimer;
};'''
content = content.replace(struct_str, '#include "EnemyBullet.h"')
with open('src/game/Game.h', 'w', encoding='utf-8') as f:
    f.write(content)

with open('src/game/Game.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

new_lines = []
skip = False
for i, line in enumerate(lines):
    if "bullet.position.x = " in line and "enemies[i]->position.x" in line:
        new_lines.append("            Vector2 startPos = { enemies[i]->position.x + (forward.x * offset), enemies[i]->position.y + (forward.y * offset) };\n")
        new_lines.append("            EnemyBullet bullet;\n")
        new_lines.append("            bullet.Initialize(startPos, forward, 250.0f);\n")
        skip = True
        continue
    
    if skip and "enemyBullets.push_back(bullet);" in line:
        new_lines.append(line)
        skip = False
        continue
        
    if skip:
        continue
        
    if "b.position.x += b.velocity.x * deltaTime;" in line:
        new_lines.append("        b.Update(deltaTime);\n")
        skip = True
        continue
        
    if skip and "b.trail.erase(b.trail.begin());" in lines[i-2] and "}" in lines[i-1] and "}" in line:
        skip = False
        continue
        
    if "if (!player.isDestroyed && CheckCollisionPointRec(b.position, player.GetHitbox()))" in line:
        new_lines.append(line)
        skip = True
        continue
        
    if skip and "b.active = false;" in line:
        new_lines.append("                b.OnHit();\n")
        skip = False
        continue
        
    if "// Rastro" in line and "for (int i = 0; i < (int)b.trail.size(); i++)" in lines[i+1]:
        new_lines.append("        b.Render();\n")
        skip = True
        continue
        
    if skip and "DrawCircleV(b.position, 2.0f, WHITE);" in lines[i-2] and "}" in lines[i-1] and "}" in line:
        skip = False
        continue
        
    if not skip:
        new_lines.append(line)
        
with open('src/game/Game.cpp', 'w', encoding='utf-8') as f:
    f.writelines(new_lines)
