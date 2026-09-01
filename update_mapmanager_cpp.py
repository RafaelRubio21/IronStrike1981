import re

with open('src/game/map/MapManager.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

old_loop = '''    for (const auto& layer : j["layers"]) {
        if (layer["type"] == "tilelayer") {
            TileLayer tl;
            tl.name = layer["name"].get<std::string>();
            tl.visible = layer["visible"].get<bool>();
            tl.opacity = layer["opacity"].get<float>();
            
            const auto& dataArr = layer["data"];
            tl.data.reserve(dataArr.size());
            for (const auto& d : dataArr) {
                tl.data.push_back(d.get<int>());
            }
            
            layers.push_back(tl);
        }
    }'''

new_loop = '''    pendingSpawns.clear();

    for (const auto& layer : j["layers"]) {
        if (layer["type"] == "tilelayer") {
            TileLayer tl;
            tl.name = layer["name"].get<std::string>();
            tl.visible = layer["visible"].get<bool>();
            tl.opacity = layer["opacity"].get<float>();
            
            const auto& dataArr = layer["data"];
            tl.data.reserve(dataArr.size());
            for (const auto& d : dataArr) {
                tl.data.push_back(d.get<int>());
            }
            
            layers.push_back(tl);
        }
        else if (layer["type"] == "objectgroup") {
            if (layer.contains("objects")) {
                for (const auto& obj : layer["objects"]) {
                    EnemySpawnData spawn;
                    spawn.x = obj.value("x", 0.0f);
                    spawn.y = obj.value("y", 0.0f);
                    
                    if (obj.contains("type") && obj["type"].is_string()) {
                        spawn.type = obj["type"].get<std::string>();
                    } else if (obj.contains("class") && obj["class"].is_string()) { // Tiled newer versions
                        spawn.type = obj["class"].get<std::string>();
                    } else {
                        spawn.type = "Tank"; // fallback
                    }
                    
                    spawn.direction = 0;
                    if (obj.contains("properties")) {
                        for (const auto& prop : obj["properties"]) {
                            if (prop["name"] == "direction") {
                                spawn.direction = prop.value("value", 0);
                            }
                        }
                    }
                    
                    pendingSpawns.push_back(spawn);
                }
            }
        }
    }'''

content = content.replace(old_loop, new_loop)

# Append PopReadySpawns implementation
pop_method = '''
std::vector<EnemySpawnData> MapManager::PopReadySpawns()
{
    std::vector<EnemySpawnData> readySpawns;
    // Iterate backwards so we can safely erase
    for (int i = (int)pendingSpawns.size() - 1; i >= 0; i--) {
        // Se a posição Y mundial do tanque for >= a posição da Câmera, 
        // significa que ele entrou na parte superior da tela (ou já estava dentro)
        if (pendingSpawns[i].y >= scrollY - 50.0f) {
            readySpawns.push_back(pendingSpawns[i]);
            pendingSpawns.erase(pendingSpawns.begin() + i);
        }
    }
    return readySpawns;
}
'''
content += pop_method

with open('src/game/map/MapManager.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
