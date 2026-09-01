import re

with open('src/game/Game.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

pattern = re.compile(r'// Gerador de Tanques.*?enemies\.push_back\(std::move\(t\)\);\s*\}', re.DOTALL)
content = pattern.sub('// Gerador de Tanques desativado (aguardando spawns via Tiled map!)', content)

with open('src/game/Game.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
