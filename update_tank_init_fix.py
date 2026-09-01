import re

with open('src/game/Tank.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

old_block = '''    cannonRotation = rotation;
    
    smokeFrame = GetRandomValue(0, 6);'''

new_block = '''    waypoints = path;
    currentWaypoint = 0;
    
    // Se tivermos waypoints, pulamos o primeiro (pois é onde nascemos)
    if (waypoints.size() > 1) {
        currentWaypoint = 1;
    }
    
    cannonRotation = rotation;
    
    smokeFrame = GetRandomValue(0, 6);'''

content = content.replace(old_block, new_block)

with open('src/game/Tank.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
