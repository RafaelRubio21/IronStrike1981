import os

with open('src/game/map/MapManager.h', 'r', encoding='utf-8') as f:
    content = f.read()

new_struct = '''
struct EnemySpawnData {
    std::string type;
    int direction;
    float x;
    float y;
};

// Estrutura para suportar'''

content = content.replace('// Estrutura para suportar', new_struct)

new_methods = '''    std::vector<EnemySpawnData> PopReadySpawns();

private:
    std::vector<EnemySpawnData> pendingSpawns;
    bool isLoaded;'''

content = content.replace('''private:
    bool isLoaded;''', new_methods)

with open('src/game/map/MapManager.h', 'w', encoding='utf-8') as f:
    f.write(content)
