import re

with open('src/game/Tank.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Fix signature
content = content.replace('void Tank::Initialize(Vector2 startPos, int spawnDirection, int tankType)\n{', 'void Tank::Initialize(Vector2 startPos, int spawnDirection, int tankType, std::vector<Vector2> path)\n{')

with open('src/game/Tank.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
