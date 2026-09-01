#include "MapManager.h"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

MapManager::MapManager() : isLoaded(false), scrollY(0.0f) {}

MapManager::~MapManager() {
    Unload();
}

bool MapManager::Load(const std::string& jsonFilePath)
{
    Unload();

    std::ifstream file(jsonFilePath);
    if (!file.is_open()) {
        std::cerr << "[MapManager] Falha ao abrir o mapa JSON: " << jsonFilePath << std::endl;
        return false;
    }

    json j;
    try {
        file >> j;
    } catch (const json::parse_error& e) {
        std::cerr << "[MapManager] Erro no parsing do JSON: " << e.what() << std::endl;
        return false;
    }

    mapWidth = j["width"].get<int>();
    mapHeight = j["height"].get<int>();
    tileWidth = j["tilewidth"].get<int>();
    tileHeight = j["tileheight"].get<int>();

    std::string basePath = "";
    size_t lastSlash = jsonFilePath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        basePath = jsonFilePath.substr(0, lastSlash + 1);
    }

    for (const auto& ts : j["tilesets"]) {
        Tileset t;
        t.firstGid = ts["firstgid"].get<int>();
        t.tileWidth = ts.value("tilewidth", tileWidth);
        t.tileHeight = ts.value("tileheight", tileHeight);
        t.singleTexture.id = 0;
        
        if (ts.contains("image")) {
            t.isMultiImage = false;
            std::string imageStr = ts["image"].get<std::string>();
            std::string imgPath = basePath + imageStr;
            t.singleTexture = LoadTexture(imgPath.c_str());
            t.columns = ts.value("columns", 1);
            tilesets.push_back(t);
        } else if (ts.contains("tiles")) {
            t.isMultiImage = true;
            for (const auto& tileObj : ts["tiles"]) {
                if (tileObj.contains("id") && tileObj.contains("image")) {
                    int localId = tileObj["id"].get<int>();
                    std::string imgStr = tileObj["image"].get<std::string>();
                    std::string imgPath = basePath + imgStr;
                    Texture2D tex = LoadTexture(imgPath.c_str());
                    t.multiTextures[localId] = tex;
                }
            }
            tilesets.push_back(t);
        } else {
            std::cerr << "[MapManager] Aviso: Tileset externo não é suportado automaticamente ou falta a tag 'image'/'tiles'." << std::endl;
        }
    }

    pendingSpawns.clear();

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
                    
                    if (obj.contains("polyline")) {
                        for (const auto& pt : obj["polyline"]) {
                            float px = spawn.x;
                            float py = spawn.y;
                            if (pt.contains("x")) px += pt["x"].get<float>();
                            if (pt.contains("y")) py += pt["y"].get<float>();
                            spawn.path.push_back({ px, py });
                        }
                    }
                    
                    pendingSpawns.push_back(spawn);
                }
            }
        }
    }

    isLoaded = true;
    scrollY = (float)(mapHeight * tileHeight) - 768.0f;
    return true;
}

void MapManager::Update(float deltaTime, float speed)
{
    if (!isLoaded) return;
    
    scrollY -= speed * deltaTime;
    if (scrollY < 0.0f) scrollY = 0.0f; 
}

void MapManager::Render() const
{
    if (!isLoaded) return;

    const float screenWidth = 1024.0f;
    const float screenHeight = 768.0f;

    // Margem de segurança para desenhar construções grandes que ultrapassam o tamanho de 1 bloco (64x64)
    int margin = 5; 

    int startY = (int)(scrollY / tileHeight) - margin;
    int endY = (int)(scrollY / tileHeight) + (int)(screenHeight / tileHeight) + margin;
    
    int startX = 0 - margin;
    int endX = (int)(screenWidth / tileWidth) + margin;

    if (startY < 0) startY = 0;
    if (endY > mapHeight) endY = mapHeight;
    if (endX > mapWidth) endX = mapWidth;

    for (const auto& layer : layers) {
        if (!layer.visible) continue;

        Color tint = {255, 255, 255, (unsigned char)(layer.opacity * 255.0f)};

        for (int y = startY; y < endY; ++y) {
            for (int x = startX; x < endX; ++x) {
                int tileIndex = y * mapWidth + x;
                int gid = layer.data[tileIndex];
                
                gid &= 0x0FFFFFFF;
                if (gid == 0) continue; 

                const Tileset* ts = nullptr;
                for (auto it = tilesets.rbegin(); it != tilesets.rend(); ++it) {
                    if (gid >= it->firstGid) {
                        ts = &(*it);
                        break;
                    }
                }

                if (ts) {
                    int localId = gid - ts->firstGid;
                    Vector2 destPos = {
                        (float)(x * tileWidth),
                        (float)(y * tileHeight) - scrollY
                    };

                    if (ts->isMultiImage) {
                        auto it = ts->multiTextures.find(localId);
                        if (it != ts->multiTextures.end()) {
                            DrawTextureV(it->second, destPos, tint);
                            
                            }
                    } else {
                        if (ts->singleTexture.id != 0) {
                            int col = localId % ts->columns;
                            int row = localId / ts->columns;
                            Rectangle sourceRec = {
                                (float)(col * ts->tileWidth),
                                (float)(row * ts->tileHeight),
                                (float)ts->tileWidth,
                                (float)ts->tileHeight
                            };
                            DrawTextureRec(ts->singleTexture, sourceRec, destPos, tint);
                        }
                    }
                }
            }
        }
    }
}

void MapManager::Unload()
{
    for (auto& ts : tilesets) {
        if (ts.singleTexture.id != 0) {
            UnloadTexture(ts.singleTexture);
            ts.singleTexture.id = 0;
        }
        for (auto& pair : ts.multiTextures) {
            if (pair.second.id != 0) {
                UnloadTexture(pair.second);
                pair.second.id = 0;
            }
        }
        ts.multiTextures.clear();
    }
    tilesets.clear();
    layers.clear();
    isLoaded = false;
}


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
