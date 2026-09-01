#pragma once
#include <raylib.h>
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

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
    void Update(float deltaTime, float speed);
    void Render() const;
    void Unload();

    int GetMapWidth() const { return mapWidth; }
    int GetTileWidth() const { return tileWidth; }

private:
    bool isLoaded;
    int mapWidth;    
    int mapHeight;   
    int tileWidth;   
    int tileHeight;  

    float scrollY;   

    std::vector<Tileset> tilesets;
    std::vector<TileLayer> layers;
};

