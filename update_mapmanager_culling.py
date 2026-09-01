import re

with open('src/game/map/MapManager.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Fix the culling range
old_culling = '''    int startY = (int)(scrollY / tileHeight);
    int endY = startY + (int)(screenHeight / tileHeight) + 2;
    
    int startX = 0;
    int endX = (int)(screenWidth / tileWidth) + 2;'''

new_culling = '''    // Margem de segurança para desenhar construções grandes que ultrapassam o tamanho de 1 bloco (64x64)
    int margin = 5; 

    int startY = (int)(scrollY / tileHeight) - margin;
    int endY = (int)(scrollY / tileHeight) + (int)(screenHeight / tileHeight) + margin;
    
    int startX = 0 - margin;
    int endX = (int)(screenWidth / tileWidth) + margin;'''

content = content.replace(old_culling, new_culling)


# Fix the Drawing alignment for MultiImage tiles
old_draw = '''                    if (ts->isMultiImage) {
                        auto it = ts->multiTextures.find(localId);
                        if (it != ts->multiTextures.end()) {
                            DrawTextureV(it->second, destPos, tint);
                        }
                    } else {'''

new_draw = '''                    if (ts->isMultiImage) {
                        auto it = ts->multiTextures.find(localId);
                        if (it != ts->multiTextures.end()) {
                            // No Tiled, imagens maiores que o grid (ex: construções) são ancoradas pela base inferior-esquerda!
                            // Se desenharmos no destPos normal (que é o top-left do grid 64x64), elas afundam na terra.
                            // Precisamos calcular onde fica o "chão" do grid e subir a altura real da imagem.
                            float bottomY = (float)((y + 1) * tileHeight) - scrollY;
                            Vector2 realPos = {
                                (float)(x * tileWidth),
                                bottomY - it->second.height
                            };
                            
                            DrawTextureV(it->second, realPos, tint);
                        }
                    } else {'''

content = content.replace(old_draw, new_draw)

with open('src/game/map/MapManager.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
