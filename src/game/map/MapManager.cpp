#include "MapManager.h"
#include "../Constants.h"
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
    objects.clear();
    hasPlayerStart = false;

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
            
            // Um mapa inconsistente do Tiled estouraria o vetor no Render()
            const size_t expected = (size_t)mapWidth * (size_t)mapHeight;
            if (tl.data.size() != expected) {
                std::cerr << "[MapManager] Aviso: camada '" << tl.name << "' tem "
                          << tl.data.size() << " tiles, esperado " << expected << ". Ajustando." << std::endl;
                tl.data.resize(expected, 0);
            }

            layers.push_back(tl);
        }
        else if (layer["type"] == "objectgroup") {
            if (layer.contains("objects")) {
                for (const auto& obj : layer["objects"]) {
                    // Objeto COM imagem é cenário (construção, decoração), não
                    // spawn de inimigo. O gid é quem diz qual imagem desenhar, e
                    // os bits altos carregam os flags de espelhamento do Tiled.
                    if (obj.contains("gid")) {
                        MapObject mo;
                        mo.gid = obj["gid"].get<int>() & 0x0FFFFFFF;
                        mo.x = obj.contains("x") ? obj["x"].get<float>() : 0.0f;
                        mo.y = obj.contains("y") ? obj["y"].get<float>() : 0.0f;
                        mo.width = obj.contains("width") ? obj["width"].get<float>() : 0.0f;
                        mo.height = obj.contains("height") ? obj["height"].get<float>() : 0.0f;

                        // Propriedades da classe Building. Sem HP a construção
                        // é só cenário. EffectDestroyed ainda não é usado.
                        if (obj.contains("properties")) {
                            for (const auto& prop : obj["properties"]) {
                                if (prop["name"] == "HP") {
                                    if (prop.contains("value") && prop["value"].is_number()) {
                                        mo.hp = prop["value"].get<int>();
                                    }
                                }
                                else if (prop["name"] == "FrameDestroyed") {
                                    if (prop.contains("value") && prop["value"].is_string()) {
                                        mo.frameDestroyed = prop["value"].get<std::string>();
                                    }
                                }
                            }
                        }
                        mo.maxHp = mo.hp;

                        // Carrega o sprite de ruína uma vez por nome: as
                        // construções repetem bastante o mesmo arquivo.
                        if (!mo.frameDestroyed.empty() &&
                            destroyedTextures.find(mo.frameDestroyed) == destroyedTextures.end())
                        {
                            std::string ruinaPath = basePath + "Buildings/" + mo.frameDestroyed + ".png";
                            Texture2D ruina = LoadTexture(ruinaPath.c_str());

                            if (ruina.id == 0) {
                                std::cerr << "[MapManager] Aviso: FrameDestroyed '" << mo.frameDestroyed
                                          << "' nao encontrado em " << ruinaPath << std::endl;
                            }
                            destroyedTextures[mo.frameDestroyed] = ruina;
                        }

                        objects.push_back(mo);
                        continue;
                    }

                    EnemySpawnData spawn;
                    spawn.x = obj.contains("x") ? obj["x"].get<float>() : 0.0f;
                    spawn.y = obj.contains("y") ? obj["y"].get<float>() : 0.0f;
                    spawn.width = obj.contains("width") ? obj["width"].get<float>() : 0.0f;
                    spawn.height = obj.contains("height") ? obj["height"].get<float>() : 0.0f;
                    spawn.quantity = 1; // Default
                    
                    if (obj.contains("type") && obj["type"].is_string()) {
                        spawn.type = obj["type"].get<std::string>();
                    } else if (obj.contains("class") && obj["class"].is_string()) { // Tiled newer versions
                        spawn.type = obj["class"].get<std::string>();
                    } else {
                        spawn.type = "Tank"; // fallback
                    }

                    // Ponto de partida do helicóptero. É só uma marcação de
                    // lugar, não gera inimigo nenhum.
                    if (spawn.type == "PlayerPosition") {
                        playerStart = { spawn.x, spawn.y };
                        hasPlayerStart = true;
                        continue;
                    }
                    
                    spawn.direction = 0;
                    if (obj.contains("properties")) {
                        for (const auto& prop : obj["properties"]) {
                            if (prop["name"] == "direction") {
                                spawn.direction = prop.value("value", 0);
                            }
                            else if (prop["name"] == "EnemyType") {
                                if (prop.contains("value") && prop["value"].is_string()) {
                                    spawn.enemyType = prop["value"].get<std::string>();
                                }
                            }
                            else if (prop["name"] == "Quantity") {
                                spawn.quantity = prop.value("value", 1);
                            }
                            else if (prop["name"] == "HP") {
                                if (prop.contains("value") && prop["value"].is_number()) {
                                    spawn.stats.hp = prop["value"].get<int>();
                                }
                            }
                            else if (prop["name"] == "Speed") {
                                if (prop.contains("value") && prop["value"].is_number()) {
                                    spawn.stats.speed = prop["value"].get<float>();
                                }
                            }
                        }
                    }

                    // Quem não declarou o EnemyType vira TANK. O Tiled só grava a
                    // propriedade quando ela difere do default da classe, então um
                    // objeto EnemyRoute deixado no padrão chega aqui sem ela.
                    if (spawn.enemyType.empty()) {
                        spawn.enemyType = "TANK";
                    }
                    
                    if (obj.contains("polyline")) {
                        for (const auto& pt : obj["polyline"]) {
                            float px = spawn.x;
                            float py = spawn.y;
                            if (pt.contains("x")) px += pt["x"].get<float>();
                            if (pt.contains("y")) py += pt["y"].get<float>();
                            spawn.path.push_back({ px, py });
                        }

                        // O Tiled guarda os vértices relativos à origem do objeto,
                        // e o primeiro NÃO é necessariamente (0,0): basta arrastar
                        // um vértice no editor que ele deixa de ser. Como o tanque
                        // nasce no início do traçado e o Tank::Initialize pula o
                        // waypoint 0 justamente por assumir que nasceu nele, a
                        // origem do spawn tem que ser o primeiro ponto da rota.
                        // Sem isso o tanque nascia fora da linha e cortava caminho
                        // direto pro segundo vértice.
                        if (!spawn.path.empty()) {
                            spawn.x = spawn.path[0].x;
                            spawn.y = spawn.path[0].y;
                        }
                    }
                    
                    pendingSpawns.push_back(spawn);
                }
            }
        }
    }

    isLoaded = true;
    scrollY = (float)(mapHeight * tileHeight) - (float)Config::SCREEN_HEIGHT;
    return true;
}

float MapManager::Update(float deltaTime, float speed)
{
    if (!isLoaded) return 0.0f;

    const float anterior = scrollY;

    scrollY -= speed * deltaTime;
    if (scrollY < 0.0f) scrollY = 0.0f;

    // Devolve quantos pixels o cenário REALMENTE andou. No fim do mapa o
    // clamp acima trava o scroll, e quem move os inimigos precisa saber
    // disso para não continuar empurrando eles para baixo sozinho.
    return anterior - scrollY;
}

const Tileset* MapManager::FindTileset(int gid) const
{
    for (auto it = tilesets.rbegin(); it != tilesets.rend(); ++it) {
        if (gid >= it->firstGid) return &(*it);
    }
    return nullptr;
}

void MapManager::RenderGround() const
{
    if (!isLoaded) return;

    const float screenWidth = (float)Config::SCREEN_WIDTH;
    const float screenHeight = (float)Config::SCREEN_HEIGHT;

    // Margem de segurança para as construções que ultrapassam 1 bloco. Como
    // elas crescem para cima, um tile abaixo da tela ainda pode aparecer nela,
    // por isso a margem sai da altura do tileset mais alto e não de um chute.
    int tallest = tileHeight;
    for (const auto& ts : tilesets) {
        if (ts.tileHeight > tallest) tallest = ts.tileHeight;
    }
    int margin = (tallest / tileHeight) + 2; 

    int startY = (int)(scrollY / tileHeight) - margin;
    int endY = (int)(scrollY / tileHeight) + (int)(screenHeight / tileHeight) + margin;
    
    int startX = 0 - margin;
    int endX = (int)(screenWidth / tileWidth) + margin;

    if (startY < 0) startY = 0;
    if (startX < 0) startX = 0; // sem isso, tileIndex fica negativo e lê fora do vetor
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

                const Tileset* ts = FindTileset(gid);

                if (ts) {
                    int localId = gid - ts->firstGid;

                    // Canto INFERIOR esquerdo da célula. O Tiled ancora o tile
                    // pela base: uma imagem mais alta que o grid cresce para
                    // cima, ela não desce. Um prédio de 129px numa célula de 64
                    // aparecia 65px abaixo do lugar por causa disso, cobrindo o
                    // que estava embaixo dele.
                    float cellLeft = (float)(x * tileWidth);
                    float cellBottom = (float)(y * tileHeight) + (float)tileHeight - scrollY;

                    if (ts->isMultiImage) {
                        auto it = ts->multiTextures.find(localId);
                        if (it != ts->multiTextures.end()) {
                            // Cada imagem da coleção tem o seu próprio tamanho
                            Vector2 destPos = { cellLeft, cellBottom - (float)it->second.height };
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
                            Vector2 destPos = { cellLeft, cellBottom - (float)ts->tileHeight };
                            DrawTextureRec(ts->singleTexture, sourceRec, destPos, tint);
                        }
                    }
                }
            }
        }
    }

}

// Distância que a sombra das construções cai. Maior que a dos tanques (8px)
// porque as casas são bem mais altas. A transparência não é definida aqui:
// quem controla é o carimbo do canvas global de sombras, no Game.
static const float BUILDING_SHADOW_OFFSET = 16.0f;

void MapManager::RenderObjectShadows() const
{
    if (!isLoaded) return;
    DrawObjects(BUILDING_SHADOW_OFFSET, BUILDING_SHADOW_OFFSET, BLACK);
}

void MapManager::RenderObjects() const
{
    if (!isLoaded) return;
    DrawObjects(0.0f, 0.0f, WHITE);
}

// ---------------------------------------------------------------
// Tile objects das object layers (as construções).
// ---------------------------------------------------------------
void MapManager::DrawObjects(float offsetX, float offsetY, Color tint) const
{
    const float screenHeight = (float)Config::SCREEN_HEIGHT;

    for (const auto& mo : objects) {
        Texture2D tex = { 0 };
        Rectangle source = { 0.0f, 0.0f, 0.0f, 0.0f };
        bool usandoRuina = false;

        if (mo.isDestroyed) {
            // Construção derrubada: entra o sprite de ruína no lugar
            auto it = destroyedTextures.find(mo.frameDestroyed);
            if (it == destroyedTextures.end() || it->second.id == 0) continue;
            tex = it->second;
            source = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
            usandoRuina = true;
        } else {
            const Tileset* ts = FindTileset(mo.gid);
            if (!ts) continue;

            int localId = mo.gid - ts->firstGid;

            if (ts->isMultiImage) {
                auto it = ts->multiTextures.find(localId);
                if (it == ts->multiTextures.end() || it->second.id == 0) continue;
                tex = it->second;
                source = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
            } else {
                if (ts->singleTexture.id == 0) continue;
                tex = ts->singleTexture;
                int col = localId % ts->columns;
                int row = localId / ts->columns;
                source = {
                    (float)(col * ts->tileWidth),
                    (float)(row * ts->tileHeight),
                    (float)ts->tileWidth,
                    (float)ts->tileHeight
                };
            }
        }

        // width/height do objeto valem quando ele foi redimensionado no editor.
        // A ruína usa o tamanho nativo dela, para não ser esticada até o
        // tamanho da casa inteira caso as duas artes tenham medidas diferentes.
        float w = (!usandoRuina && mo.width > 0.0f) ? mo.width : source.width;
        float h = (!usandoRuina && mo.height > 0.0f) ? mo.height : source.height;

        // O y do Tiled é a BASE do objeto, então o topo fica em y - altura
        float topY = (mo.y - scrollY) - h;

        // Culling: nada a fazer se está todo acima ou todo abaixo da tela
        if (topY + h < 0.0f || topY > screenHeight) continue;

        Rectangle dest = { mo.x + offsetX, topY + offsetY, w, h };
        DrawTexturePro(tex, source, dest, { 0.0f, 0.0f }, 0.0f, tint);
    }
}

// ---------------------------------------------------------------
// Construções destrutíveis
// ---------------------------------------------------------------

// Encolhe a hitbox em relação à arte: as casas têm bastante pixel
// transparente em volta do telhado, e sem isso o tiro acerta o vazio.
static const float BUILDING_HITBOX_SCALE = 0.85f;

bool MapManager::IsDestructible(int index) const
{
    if (index < 0 || index >= (int)objects.size()) return false;
    return objects[index].maxHp > 0;
}

bool MapManager::IsObjectDestroyed(int index) const
{
    if (index < 0 || index >= (int)objects.size()) return true;
    return objects[index].isDestroyed;
}

Rectangle MapManager::GetObjectHitbox(int index) const
{
    if (index < 0 || index >= (int)objects.size()) return { 0.0f, 0.0f, 0.0f, 0.0f };

    const MapObject& mo = objects[index];

    float w = mo.width;
    float h = mo.height;

    // Objeto sem tamanho no editor: cai no tamanho nativo da imagem
    if (w <= 0.0f || h <= 0.0f) {
        const Tileset* ts = FindTileset(mo.gid);
        if (ts && ts->isMultiImage) {
            auto it = ts->multiTextures.find(mo.gid - ts->firstGid);
            if (it != ts->multiTextures.end()) {
                if (w <= 0.0f) w = (float)it->second.width;
                if (h <= 0.0f) h = (float)it->second.height;
            }
        }
    }

    float hitW = w * BUILDING_HITBOX_SCALE;
    float hitH = h * BUILDING_HITBOX_SCALE;

    // O y do Tiled é a BASE do objeto; a caixa fica centrada na arte
    float topY = (mo.y - scrollY) - h;

    return {
        mo.x + (w - hitW) / 2.0f,
        topY + (h - hitH) / 2.0f,
        hitW,
        hitH
    };
}

bool MapManager::DamageObject(int index, int damage)
{
    if (index < 0 || index >= (int)objects.size()) return false;

    MapObject& mo = objects[index];
    if (mo.maxHp <= 0 || mo.isDestroyed) return false;

    mo.hp -= damage;
    if (mo.hp > 0) return false;

    mo.hp = 0;
    mo.isDestroyed = true;
    return true; // só o tiro que derrubou devolve true
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
    for (auto& pair : destroyedTextures) {
        if (pair.second.id != 0) UnloadTexture(pair.second);
    }
    destroyedTextures.clear();

    tilesets.clear();
    layers.clear();
    objects.clear();
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
