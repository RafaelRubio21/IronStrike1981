import re

with open('src/game/map/MapManager.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

old_pts = '''                    if (obj.contains("polyline")) {
                        for (const auto& pt : obj["polyline"]) {
                            float px = spawn.x + pt.value("x", 0.0f);
                            float py = spawn.y + pt.value("y", 0.0f);
                            spawn.path.push_back({ px, py });
                        }
                    }'''

new_pts = '''                    if (obj.contains("polyline")) {
                        for (const auto& pt : obj["polyline"]) {
                            float px = spawn.x;
                            float py = spawn.y;
                            if (pt.contains("x")) px += pt["x"].get<float>();
                            if (pt.contains("y")) py += pt["y"].get<float>();
                            spawn.path.push_back({ px, py });
                        }
                    }'''

content = content.replace(old_pts, new_pts)

with open('src/game/map/MapManager.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
