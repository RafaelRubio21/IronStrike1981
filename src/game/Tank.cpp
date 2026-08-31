#include "Tank.h"

// =================================================================
// CONFIGURAÇÕES GERAIS DOS TANQUES (Fique à vontade para alterar!)
// =================================================================
static int TANK_MAX_HP = 5;               // Tiros necessarios para destruir
static float TANK_SPEED = 60.0f;          // Velocidade de movimento
static float TANK_FRICTION = 150.0f;      // Força de frenagem ao ser destruido (derrapada)
static float TANK_VOL_EXPLOSION = 1.0f;   // Volume da explosão (0.0 a 1.0)
static float TANK_VOL_ENGINE = 0.1f;     // Volume do motor (0.0 a 1.0)
static float TANK_VOL_SHOOTING = 0.4f;    // Volume do tiro inimigo (0.0 a 1.0)
// =================================================================

// Variáveis globais/estáticas para carregar texturas e áudios apenas uma vez
static Texture2D tankFrames[4] = {0};
static Texture2D tankDestroyedFrame = {0};
static bool tankTexturesLoaded = false;

static Sound tankExplodingSnd = {0};
static Sound tankMovingSnd = {0};
static Sound tankShootingSnd = {0};
static bool tankAudioLoaded = false;

void Tank::Initialize(Vector2 startPos, int spawnDirection)
{
    position = startPos;
    isDestroyed = false;
    isActive = true;
    
    scale = 1.0f; // 1.0 = Tamanho original de 67x94
    currentFrame = 0;
    frameTimer = 0.0f;
    
    hp = TANK_MAX_HP; 
    hitTimer = 0.0f;
    
    // O Sprite aponta nativamente para BAIXO
    if (spawnDirection == 0)
    {
        velocity = { 0.0f, TANK_SPEED }; // Desce
        rotation = 0.0f;
    }
    else if (spawnDirection == 1)
    {
        velocity = { TANK_SPEED, 0.0f }; // Vai pra direita
        rotation = -90.0f; // Vira pra direita
    }
    else if (spawnDirection == 2)
    {
        velocity = { -TANK_SPEED, 0.0f }; // Vai pra esquerda
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
    
    if (!tankAudioLoaded)
    {
        const char* audioPaths[] = {
            "assets/audio/tank/",
            "../../assets/audio/tank/",
            "../../../assets/audio/tank/"
        };
        
        for (int i = 0; i < 3; i++)
        {
            if (tankExplodingSnd.frameCount == 0)
            {
                tankExplodingSnd = LoadSound(TextFormat("%sexploding.ogg", audioPaths[i]));
                tankMovingSnd = LoadSound(TextFormat("%smoving.ogg", audioPaths[i]));
                tankShootingSnd = LoadSound(TextFormat("%sshotting.ogg", audioPaths[i]));
            }
        }
        
        // Mantém os sons mais baixos já que os tanques estão lá embaixo no mapa
        if (tankExplodingSnd.frameCount != 0) SetSoundVolume(tankExplodingSnd, TANK_VOL_EXPLOSION);
        if (tankMovingSnd.frameCount != 0) SetSoundVolume(tankMovingSnd, TANK_VOL_ENGINE); 
        if (tankShootingSnd.frameCount != 0) SetSoundVolume(tankShootingSnd, TANK_VOL_SHOOTING);
        
        tankAudioLoaded = true;
    }
}

void Tank::TakeDamage(int damage)
{
    if (isDestroyed) return;
    
    hp -= damage;
    hitTimer = 0.05f; // Pisca de vermelho por 0.05 segundos ao ser alvejado
    
    if (hp <= 0)
    {
        Destroy();
    }
}

void Tank::Destroy()
{
    if (!isDestroyed)
    {
        isDestroyed = true;
        
        // Toca o som de explosão ao ser destruído
        if (tankExplodingSnd.frameCount != 0) PlaySound(tankExplodingSnd);
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
    
    if (hitTimer > 0.0f) hitTimer -= deltaTime;
    
    // Movimento
    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;
    
    if (isDestroyed)
    {
        // Atrito pesado: o tanque derrapa no chão (ferro com terra) até parar 
        // Freia o eixo X
        if (velocity.x > 0.0f) { velocity.x -= TANK_FRICTION * deltaTime; if (velocity.x < 0.0f) velocity.x = 0.0f; }
        else if (velocity.x < 0.0f) { velocity.x += TANK_FRICTION * deltaTime; if (velocity.x > 0.0f) velocity.x = 0.0f; }

        // Freia o eixo Y
        if (velocity.y > 0.0f) { velocity.y -= TANK_FRICTION * deltaTime; if (velocity.y < 0.0f) velocity.y = 0.0f; }
        else if (velocity.y < 0.0f) { velocity.y += TANK_FRICTION * deltaTime; if (velocity.y > 0.0f) velocity.y = 0.0f; }
    }
    else
    {
        // Animação das lagartas (só se estiver vivo)
        frameTimer += deltaTime;
        if (frameTimer >= 0.05f) // 20 FPS (Mais rápido e suave)
        {
            frameTimer = 0.0f;
            currentFrame++;
            if (currentFrame >= 4) currentFrame = 0; 
        }
        
        // Mantém o som do motor tocando se o tanque estiver se movendo
        if (tankMovingSnd.frameCount != 0)
        {
            if (!IsSoundPlaying(tankMovingSnd))
            {
                PlaySound(tankMovingSnd);
            }
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
    
    // Se tomou um tiro recentemente, pisca de vermelho!
    Color tintColor = (hitTimer > 0.0f && !isDestroyed) ? RED : WHITE;
    
    DrawTexturePro(tex, sourceRec, destRec, origin, rotation, tintColor);
}
