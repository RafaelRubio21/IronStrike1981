import re

with open('src/game/map/MapManager.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

old_block = '''                    if (obj.contains("properties")) {
                        for (const auto& prop : obj["properties"]) {
                            if (prop["name"] == "direction") {
                                spawn.direction = prop.value("value", 0);
                            }
                        }
                    }
                    
                    pendingSpawns.push_back(spawn);'''

new_block = '''                    if (obj.contains("properties")) {
                        for (const auto& prop : obj["properties"]) {
                            if (prop["name"] == "direction") {
                                spawn.direction = prop.value("value", 0);
                            }
                        }
                    }
                    
                    if (obj.contains("polyline")) {
                        for (const auto& pt : obj["polyline"]) {
                            float px = spawn.x + pt.value("x", 0.0f);
                            float py = spawn.y + pt.value("y", 0.0f);
                            spawn.path.push_back({ px, py });
                        }
                    }
                    
                    pendingSpawns.push_back(spawn);'''

content = content.replace(old_block, new_block)

with open('src/game/map/MapManager.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
