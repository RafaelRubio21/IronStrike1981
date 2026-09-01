import re

with open('src/game/Tank.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

old_draw = '''void Tank::DrawBody() const
{
    if (!isActive || !tankTexturesLoaded) return;
    
    // Corpo'''

new_draw = '''void Tank::DrawBody() const
{
    if (!isActive || !tankTexturesLoaded) return;
    
    // DEBUG: Desenha os waypoints do tanque!
    if (!isDestroyed && !waypoints.empty() && currentWaypoint < waypoints.size()) {
        for (int i = currentWaypoint; i < waypoints.size(); i++) {
            DrawCircle((int)waypoints[i].x, (int)waypoints[i].y, 5, RED);
            if (i > currentWaypoint) {
                DrawLineEx(waypoints[i-1], waypoints[i], 2.0f, RED);
            } else {
                DrawLineEx(position, waypoints[i], 2.0f, RED);
            }
        }
    }
    
    // Corpo'''

content = content.replace(old_draw, new_draw)

with open('src/game/Tank.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
