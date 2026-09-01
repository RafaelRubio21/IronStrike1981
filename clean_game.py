import sys

with open('src/game/Game.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

new_lines = []
skip = False
for i, line in enumerate(lines):
    if "static Texture2D smokeFrames" in line:
        skip = True
    if skip and "static float SMOKE_OFFSET_Y" in line:
        skip = False
        continue
    
    if skip:
        continue
        
    if "// Carrega a Animação de Fumaça" in line:
        skip = True
        
    if skip and "smokeLoaded = true;" in line:
        # Next line is closing brace
        skip = False
        continue
        
    if skip and "}" in line and i > 0 and "smokeLoaded = true;" in lines[i-1]:
        continue
        
    if "explosionManager.Initialize();" in line:
        new_lines.append(line)
        new_lines.append("    smokeManager.Initialize();\n")
        continue
        
    if "// ETAPA 5: DESENHAR FUMAÇAS" in line:
        new_lines.append(line)
        new_lines.append("    for (const auto& e : enemies)\n")
        new_lines.append("    {\n")
        new_lines.append("        if (e->isDestroyed) smokeManager.Render(e->position, e->smokeFrame);\n")
        new_lines.append("    }\n")
        skip = True
        continue
        
    if skip and "}" in line and i > 0 and "DrawTexturePro(sTex," in lines[i-2]:
        skip = False
        continue
        
    if not skip:
        new_lines.append(line)

with open('src/game/Game.cpp', 'w', encoding='utf-8') as f:
    f.writelines(new_lines)
