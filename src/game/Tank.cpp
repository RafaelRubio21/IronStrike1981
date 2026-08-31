#include "Tank.h"

// Variáveis globais/estáticas para carregar a textura na memória apenas uma vez
// e não travar o jogo carregando do HD toda vez que um inimigo novo nasce
static Texture2D tankFrames[4] = {0};
static Texture2D tankDestroyedFrame = {0};
static bool tankTexturesLoaded = false;

void Tank::Initialize(Vector2 startPos, int spawnDirection)
{
    position = startPos;
    isDestroyed = false;
    isActive = true;
    
    scale = 1.0f; // 1.0 = Tamanho original de 67x94
    currentFrame = 0;
    frameTimer = 0.0f;
    
    // O Sprite aponta nativamente para BAIXO
    if (spawnDirection == 0)
    {
        velocity = { 0.0f, 60.0f }; // Desce
        rotation = 0.0f;
    }
    else if (spawnDirection == 1)
    {
        velocity = { 60.0f, 0.0f }; // Vai pra direita
        rotation = -90.0f; // Vira pra direita
    }
    else if (spawnDirection == 2)
    {
        velocity = { -60.0f, 0.0f }; // Vai pra esquerda
        rotation = 90.0f; // Vira pra esquerda
    }

    if (!tankTexturesLoaded)
    {
        // Arrays com os possíveis caminhos de diretório para o Visual Studio
        const char* paths[] = {
            "assets/sprites/enemies/tank/",
            "../../assets/sprites/enemies/tank/",
            "../../../assets/sprites/enemies/tank/"
        };
        
        for (int i = 0; i < 3; i++)
        {
            if (tankFrames[0].id == 0) // Se ainda nao achou
            {
                tankFrames[0] = LoadTexture(TextFormat("%stank1.png", paths[i]));
                tankFrames[1] = LoadTexture(TextFormat("%stank2.png", paths[i]));
                tankFrames[2] = LoadTexture(TextFormat("%stank3.png", paths[i]));
                tankFrames[3] = LoadTexture(TextFormat("%stank4.png", paths[i]));
                tankDestroyedFrame = LoadTexture(TextFormat("%stank_destroyed.png", paths[i]));
            }
        }
        tankTexturesLoaded = true;
    }
}

void Tank::Destroy()
{
    if (!isDestroyed)
    {
        isDestroyed = true;
        velocity = { 0.0f, 0.0f }; // Para de andar
    }
}

Rectangle Tank::GetHitbox() const
{
    float width = 67.0f * scale;
    float height = 94.0f * scale;
    // Como a nossa origin é no meio, subtraímos metade para gerar o retângulo a partir do canto superior esquerdo
    return { position.x - (width / 2.0f), position.y - (height / 2.0f), width, height };
}

void Tank::Update(float deltaTime)
{
    if (!isActive) return;
    
    // Movimento
    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;
    
    // Animação das lagartas (só se estiver vivo)
    if (!isDestroyed)
    {
        frameTimer += deltaTime;
        if (frameTimer >= 0.05f) // 20 FPS (Mais rápido e suave)
        {
            frameTimer = 0.0f;
            currentFrame++;
            if (currentFrame >= 4) currentFrame = 0; 
        }
    }
    
    // Desativa se sair muito da tela
    if (position.x < -100 || position.x > 1200 || position.y > 900)
    {
        isActive = false;
    }
}

void Tank::DrawShadows() const
{
    if (!isActive || !tankTexturesLoaded) return;
    
    Texture2D tex = isDestroyed ? tankDestroyedFrame : tankFrames[currentFrame];
    
    float width = (float)tex.width * scale;
    float height = (float)tex.height * scale;
    
    Rectangle sourceRec = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
    
    Vector2 shadowOffset = { 8.0f * scale, 8.0f * scale };
    Rectangle destRec = { position.x + shadowOffset.x, position.y + shadowOffset.y, width, height };
    Vector2 origin = { width / 2.0f, height / 2.0f };
    
    DrawTexturePro(tex, sourceRec, destRec, origin, rotation, BLACK);
}

void Tank::DrawBody() const
{
    if (!isActive || !tankTexturesLoaded) return;
    
    Texture2D tex = isDestroyed ? tankDestroyedFrame : tankFrames[currentFrame];
    
    float width = (float)tex.width * scale;
    float height = (float)tex.height * scale;
    
    Rectangle sourceRec = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
    Rectangle destRec = { position.x, position.y, width, height };
    Vector2 origin = { width / 2.0f, height / 2.0f };
    
    DrawTexturePro(tex, sourceRec, destRec, origin, rotation, WHITE);
}
