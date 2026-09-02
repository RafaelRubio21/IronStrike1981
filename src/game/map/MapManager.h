#pragma once
#include <raylib.h>
#include "../EnemyBase.h"
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>


struct EnemySpawnData {
    std::string type;
    std::string enemyType;
    int direction;
    float x;
    float y;
    float width;
    float height;
    int quantity;
    EnemyStats stats; // HP e Speed vindos da classe EnemyRoute
    std::vector<Vector2> path;
};

// Estrutura para suportar tanto Tileset de 1 imagem só (spritesheet) quanto coleção de imagens (varias imagens soltas)
struct Tileset {
    int firstGid;        
    
    // Tipo 1: Spritesheet único
    Texture2D singleTexture;
    int columns;
    
    // Tipo 2: Coleção de imagens soltas (Tiled "Collection of Images")
    // Mapeia o ID local do Tile para uma Textura separada
    std::map<int, Texture2D> multiTextures;

    bool isMultiImage;

    int tileWidth;
    int tileHeight;
};

// Objeto com imagem colocado numa object layer do Tiled (tile object), como as
// construções. Diferente de um objeto retangular, o Tiled ancora este pelo canto
// INFERIOR esquerdo, e o x/y aqui é o do editor, em coordenadas de mundo.
struct MapObject {
    int gid;
    float x;
    float y;
    float width;   // 0 = usar o tamanho nativo da imagem
    float height;
};

// Estrutura para cada Camada de Tiles (Tile Layer)
struct TileLayer {
    std::string name;
    std::vector<int> data; 
    bool visible;
    float opacity;
};

class MapManager
{
public:
    MapManager();
    ~MapManager();

    bool Load(const std::string& jsonFilePath);
    // Retorna quantos pixels o cenário andou de fato neste frame
    float Update(float deltaTime, float speed);

    // O desenho é em três etapas porque a sombra das construções precisa cair
    // ENTRE o chão e a própria construção: chão -> sombras -> construções.
    void RenderGround() const;
    void RenderObjectShadows() const;
    void RenderObjects() const;

    void Unload();

    int GetMapWidth() const { return mapWidth; }
    int GetTileWidth() const { return tileWidth; }
    float GetScrollY() const { return scrollY; }

    // Objeto "PlayerPosition" do mapa: onde o helicóptero começa (mundo)
    bool HasPlayerStart() const { return hasPlayerStart; }
    Vector2 GetPlayerStart() const { return playerStart; }

    std::vector<EnemySpawnData> PopReadySpawns();

private:
    std::vector<EnemySpawnData> pendingSpawns;

    Vector2 playerStart = { 0.0f, 0.0f };
    bool hasPlayerStart = false;

    bool isLoaded;
    int mapWidth;    
    int mapHeight;   
    int tileWidth;   
    int tileHeight;  

    float scrollY;   

    // Acha o tileset dono de um gid (o de maior firstGid que ainda cabe nele)
    const Tileset* FindTileset(int gid) const;

    // Percorre os tile objects uma vez só; a sombra é o mesmo desenho
    // deslocado e pintado de preto.
    void DrawObjects(float offsetX, float offsetY, Color tint) const;

    std::vector<Tileset> tilesets;
    std::vector<TileLayer> layers;
    std::vector<MapObject> objects; // construções e demais tile objects
};





