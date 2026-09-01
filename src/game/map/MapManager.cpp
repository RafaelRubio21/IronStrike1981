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

    int startY = (int)(scrollY / tileHeight);
    int endY = startY + (int)(screenHeight / tileHeight) + 2;
    
    int startX = 0;
    int endX = (int)(screenWidth / tileWidth) + 2;

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

